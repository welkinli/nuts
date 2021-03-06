#ifndef	_WNS_ES_ACC_PLUGIN_albert_
#define	_WNS_ES_ACC_PLUGIN_albert_	1

#include	"eservice_user.h"
#include	"common_albert.h"
#include	"program_utils.h"
#include	"socket_utils.h"
#include	"ss_list_utils.h"
#include    "qos_client.h"
#include	<vector>
#include   <set>

#define	WNS_ACC_LB_HTTP_CLIENT_IP_FIELD	"X-Forwarded-For-Pound:"
#define	WNS_ACC_AKAMAI_HTTP_CLIENT_IP_FIELD	"HTTP_TRUE_CLIENT_IP:"
#define	WNS_ACC_CDN_HTTP_CLIENT_IP_FIELD	"Qvia:"
#define	WNS_ACC_CDN_HTTP_CLIENT_IP_HEX_LEN	40

/* 2M */
#define	WNS_ACC_MAX_VALID_PKG_LEN	2097152

/* 20M */
#define	WNS_ACC_MAX_VALID_UNCOMP_PKG_LEN	20971520

/* 10s timeout */
#define	WNS_ACC_MAX_TSP_PROCESS_SECONDS	10

/* 2s timeout */
#define	WNS_ACC_MAX_CONN_CONNECTING_SECONDS	2

#define	WNS_ACC_MAX_LISTEN_PORT	10


#if 1
/* 1500s timeout */
//#define	WNS_ACC_DEFAULT_CONN_IDLE_SECONDS	1500
#define	WNS_ACC_DEFAULT_CONN_IDLE_SECONDS	1230
#else
/* 400s timeout */
#define	WNS_ACC_MAX_CONN_IDLE_SECONDS	400
#endif

#define	WNS_ACC_SAFE_LEN	2048

#define	WNS_ACC_MAX_APPID_NUM	200

#define	WNS_ACC_TASK_STRUCT_SIGN	0x781224

#define	WNS_ACC_TASK_POS_FREE_LIST	0
#define	WNS_ACC_TASK_POS_NOLIST	1
#define	WNS_ACC_TASK_POS_WAITING	2

#define	WNS_SIGN_STR	"nut\0"
#define	WNS_SIGN_INT	0x74756e//0x736e77
#define	WNS_INNER_SIGN	0x01

#define	WNS_SIGN_FE_INT	 0xa74756e//0xa736e77
//#define	WNS_FE_PROTOCOL_LEN	4
#define	WNS_FE_PROTOCOL_LEN	8

#define	WNS_FLAG_CLIENT_EXISTS_BIT	0x1
#define	WNS_FLAG_AKAMAI_BIT	0x2
#define	WNS_FLAG_HTTP_BIT	0x4
#define	WNS_FLAG_ORIGIN_DATA	0x8
#define WNS_FLAG_CLOG_BIT      0x10

#define	SOCKID_SHIFT    32
#define	SOCKID_MASK     0xffffffff
#define	RANDID_SHIFT    0
#define	RANDID_MASK     0xffffffff

#define make_client_id(eup32bit)   (\
    (((uint64_t)(eup32bit) & SOCKID_MASK) << SOCKID_SHIFT) |\
    (((uint64_t)(random()) & RANDID_MASK) << RANDID_SHIFT))

#define get_eup(clientid) (((clientid) >> SOCKID_SHIFT) & SOCKID_MASK)



#pragma pack(1)
typedef enum {
	CMD_LOGIN =1,
	CMD_PING = 1000,
	CMD_BROADCAST = 1001,
}REQUEST_CMD;
/*
typedef enum{
	CMD_LOGIN_STRING = "login",
	CMD_PING_STRING = "ping",
}REQUEST_CMD_STR;
*/
typedef enum{
	ACC_FLAG_BROADCAST=1,
}ACC_FLAG;
#define getcmdstr(cmd) ""

typedef	struct {
	uint32_t Wns_sign;
	uint32_t Len;
} wns_bin_psy_header_t;

typedef	struct {
	uint32_t Wns_fe_sign;
} wns_fe_psy_header_t;

typedef	struct {
	char post[4];
} wns_post_psy_header_t;

typedef	struct {
	char get[3];
} wns_get_psy_header_t;

typedef	struct {
	uint8_t        SOH;
	uint32_t       Len;
} wns_inner_psy_header_t;

typedef struct {
	char		Version;
	char		TTL;
	uint16_t	Header_length;
	uint32_t	Dst_ip;
	uint16_t	Dst_port;
	uint32_t	User_ip;
	uint16_t	User_port;
} wns_cdnproxy_psy_header_t;

typedef	union {
	wns_bin_psy_header_t wns_protocol;
	wns_post_psy_header_t wns_post_protocol;
	wns_get_psy_header_t wns_get_protocol;
	wns_inner_psy_header_t wns_push_protocol;
	wns_fe_psy_header_t wns_fe_protocol;
	wns_cdnproxy_psy_header_t wns_cdnproxy_protocol;
} wns_psy_header_t;

#pragma pack()

typedef	union {
	struct _ip_port {
		in_addr_t ip;
		uint32_t port;
	} ip_port;
	
	uint64_t key64;
} acc_l5conn_key_t;

class client_reserved_info_t {
	public:
		std::string qua;
		uint32_t cmd;
		uint32_t seq;
		int req_size;
		uint32_t user_ip;
		uint32_t host_ip;
		int pro_version;
		int appid;
		uint32_t uin;
		uint64_t key;
		uint32_t server_ip;
		int protocol;
		QOSREQUEST qos;
};

/*
 * 由框架提供的eup来生成,同时把eup存到自己的成员eup中
 */
typedef	struct {
	DBL_LIST_HEAD_STRUCT(list);
	struct eservice_unit_t *eup;		/* client eup ,connection info ,for sending out and receiving */
	uint32_t sign;
	int check_header;
	uint32_t id;
	uint32_t tsp_seq;
	int need_not_rsp;
	
	struct eservice_buf_t *input_ebp;         /*//带有数据包的连接信息,从eup中解出来,eup由框架提供 */

#ifndef	ESERVICE_SINGLE_OUTPUT
	ss_list_head_struct output_ebp_list;   /*回传数据队列*/
#else
	struct eservice_buf_t *output_ebp;
#endif
	int already_send_respone;
	time_t use_time;				/* use time */
	int list_position;				/* task struct current position  表示队列类型 空闲或者是等待处理*/
	int result;
	int ultimate_result;				/* 0 = success, 1 = failed, 2 = timeout */

	wns_psy_header_t wns_psy_head;
	int protocol_type;				/* 1 = wns, 2 = POST, 3 = GET, 4 = push */
	host32_t client_ip;
    uint16_t client_port;
	int is_akamai;
	uint64_t client_id;

	client_reserved_info_t *client_info_p;	/* client reserve info */
	int cmd_can_retry;
	int error_code;
} wns_acc_task_struct;

class wns_acc_connect_user_context_t {
	public:
		std::string last_qua;

		/* for session hash */
		uint32_t session_hash;
		std::string qua;
		std::string device_info;
		std::vector<char> A2;
		int datacomed;
		uint64_t uin;
        uint32_t proxy_cdnproxy_user_ip;
};


typedef	struct {
	uint32_t task_num;
	int dest_modid;
	int dest_cmd;
	int idle_clean_seconds;
	int fast_idle_clean_seconds;
	int highest_idle_clean_seconds;
	int firstnodata_idle_clean_seconds;
	int client_retry;
	int sac_agent_flag;
    int clean_clog;
    std::set<uint64_t> uins;
    std::set<std::string> cmds;
    int other_dest_modid;
    int other_dest_cmd;
    int max_pkg_size;
    int max_group_user_num;
} wns_acc_main_config_t;


typedef	struct {
	int used;
	uint32_t bin_comm_connection_num;
	uint32_t bin_pin_connection_num;
	uint32_t bin_client_data_recved;
	uint32_t http_comm_connection_num;
	uint32_t http_pin_connection_num;
	uint32_t http_client_data_recved;
} plugin_protocol_statistic_t;

typedef	struct {
	uint32_t first_no_data_num;
	uint32_t comm_idle_time_num;
	uint32_t fast_idle_time_num;
	uint32_t highest_idle_time_num;
}plugin_idle_statistic_t;

typedef struct {
	uint32_t min15_20_user_ilde_num;
	uint32_t min10_15_user_ilde_num;
	uint32_t min5_10_user_ilde_num;
	uint32_t min0_5_user_ilde_num;
} plugin_user_distribute;

typedef	struct {
	int used;
	uint32_t port;
	uint32_t bin_prot_num;
	uint32_t http_prot_num;
} plugin_port_statistic_t;

typedef	struct {
	uint32_t req_num;
	uint32_t rsp_num;
	uint32_t dispatch_req_num;
	uint32_t dispatch_rsp_num;
	uint32_t push_num;
	plugin_port_statistic_t port_statistic_array[WNS_ACC_MAX_LISTEN_PORT];
} wns_acc_30s_statistic_t;

typedef	struct {
	uint32_t push_num;
	uint32_t less_30;
	uint32_t s_30_60;
	uint32_t s_60_180;
	uint32_t s_180_300;
	uint32_t s_300_600;
	uint32_t s_600_900;
	uint32_t s_900_1200;
	uint32_t s_1200_1500;

	uint32_t init_less_30;
	uint32_t init_s_30_60;
	uint32_t init_s_60_180;
	uint32_t init_s_180_300;
	uint32_t init_s_300_600;
	uint32_t init_s_600_900;
	uint32_t init_s_900_1200;
	uint32_t init_s_1200_1500;
} wns_acc_30s_push_t;

typedef	struct {
	int appid_num;
	int appid_array[WNS_ACC_MAX_APPID_NUM];
} wns_dynamic_config_t;

#endif

