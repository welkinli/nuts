#ifndef	_WNS_ES_ACC_PLUGIN_albert_
#define	_WNS_ES_ACC_PLUGIN_albert_	1

#include	"eservice_user.h"
#include	"common_albert.h"
#include	"program_utils.h"
#include "qmf_protocal_inter.h"
#include	"socket_utils.h"
#include	"ss_list_utils.h"
#include    "qos_client.h"
#include	<vector>
#include   <set>

#define	SOCKID_SHIFT    32
#define	SOCKID_MASK     0xffffffff
#define	RANDID_SHIFT    0
#define	RANDID_MASK     0xffffffff

#define make_client_id(eup32bit)   (\
    (((uint64_t)(eup32bit) & SOCKID_MASK) << SOCKID_SHIFT) |\
    (((uint64_t)(random()) & RANDID_MASK) << RANDID_SHIFT))

#define get_eup(clientid) (((clientid) >> SOCKID_SHIFT) & SOCKID_MASK)

#define PT_SWAP_DDWORD(x)  \
    ((((x) & 0xff00000000000000llu) >> 56) | \
     (((x) & 0x00ff000000000000llu) >> 40) | \
     (((x) & 0x0000ff0000000000llu) >> 24) | \
     (((x) & 0x0000ff0000000000llu) >> 24) | \
     (((x) & 0x000000ff00000000llu) >> 8) | \
     (((x) & 0x00000000ff000000llu) << 8) | \
     (((x) & 0x0000000000ff0000llu) << 24) | \
     (((x) & 0x000000000000ff00llu) << 40) | \
     (((x) & 0x00000000000000ffllu) << 56) )

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PT_HTONLL(x) PT_SWAP_DDWORD(x)
#else
#define PT_HTONLL(x) (x)
#endif

#ifndef htonll
#define htonll(x) PT_HTONLL(x)
#endif

#ifndef ntohll
#define ntohll(x) htonll(x)
#endif

#define	WNS_INNER_SIGN	0x74756e

typedef enum {
	CMD_LOGIN =1,
	CMD_PING = 1000,
	CMD_BROADCAST=1001,
	CMD_TALK = 1002,
}REQUEST_CMD;



typedef struct {
	DBL_LIST_HEAD_STRUCT(list);
	uint64_t uin;
	uint64_t key;
	time_t create_time;
	int seat_id;
	int group_id;
	int section_id;
	uint64_t chip_total;
	int status;  //0,未分配，1使用中
	//char name[128];
	group_info_t *pgroup;
	void init(){
		uin=0;
		key=0;
		seat_id=-1;
		group_id=0;
		section_id=0;
		chip_total=0;
		status=0;
		pgroup=NULL;
	}
	void reset(){
		status=0;
		seat_id=-1;
		group_id=0;
		section_id=0;
		chip_total=0;
		pgroup=NULL;
	}

}user_info_t;

typedef struct {
	int section_id;
	char section_name[128];
	//动态数据
	int now_group_count;
	int now_user_count;
	//静态数据
	int max_group_count;
	int max_group_user_count;
	int init_bet_count; //初始下注大小
    int min_bet_count; //跟注下限
    int max_bet_count;//跟注上限
}section_info_t;

#define MAX_UIN_NUM 16
typedef struct {
	DBL_LIST_HEAD_STRUCT(list);
	int section_id;//大区id
	//动态数据
	int group_id;
	section_info_t *psection;
	time_t create_time;
	int now_user_count;
	uint64_t uins[MAX_UIN_NUM];
	//静态数据
	int max_group_user_count; //最大用户数目
    int init_bet_count; //初始下注大小
    int min_bet_count; //跟注下限
    int max_bet_count;//跟注上限
}group_info_t;

//如果要分区配置的话，可以根据使用plugin_main_config_t[] 来代表各个区的配置，简洁易用
typedef	struct {
	uint32_t task_num;
	int section_id;
	char db_ipchar[16];
	int db_port;
	char redis_ipchar[16];
	int redis_port;
	int client_retry;
    int max_pkg_size;
    int max_group_user_count;
    int max_total_user_count;
    int init_bet_count; //初始下注大小
    int min_bet_count; //跟注下限
    int max_bet_count;//跟注上限
    static void init(plugin_main_config_t st){
    	memset(&st,0,sizeof(st));
    }
} plugin_main_config_t;

typedef	struct {
	uint32_t Wns_sign;
	uint32_t Len;
	uint8_t Ver;
	uint32_t Cmd;
	uint64_t Uin;
	uint64_t Key;
	uint32_t BodyLen;
	void *Body;
	void decode(){
		Len=ntohl(Len);
		Cmd=ntohl(Cmd);
		Uin=ntohll(Uin);
		Key=ntohll(Key);
		BodyLen=ntohll(BodyLen);
	}
	void encode(){
		Len=htonl(Len);
		Cmd=htonl(Cmd);
		Uin=htonll(Uin);
		Key=htonll(Key);
		BodyLen=htonll(BodyLen);

	}
} plugin_header_t;

typedef struct{
	plugin_header_t head;
	int rspcode;
	char rspinfo[128];
	void encode(){
		head.encode();
		rspcode=htonl(rspcode);
	}

}plugin_respone_head_t;

typedef struct {

	DBL_LIST_HEAD_STRUCT(list);
	plugin_header_t *plugin_head;
	struct eservice_unit_t *eup;		/* client eup ,connection info ,for sending out and receiving */
	uint32_t tsp_seq;
	int need_not_rsp;
	void* puser;	                       /*user info  */

	struct eservice_buf_t *input_ebp;         /*带有数据包的连接信息,从eup中解出来,eup由框架提供 */

	ss_list_head_struct output_ebp_list;   /*回传数据队列*/

	int already_send_respone;
	time_t use_time;				/* use time */
	int list_position;				/* task struct current position  表示队列类型 空闲或者是等待处理*/
	int result;
	int ultimate_result;				/* 0 = success, 1 = failed, 2 = timeout */

	int protocol_type;				/* 1 = wns, 2 = POST, 3 = GET, 4 = push */
	uint64_t client_id;				//记录的client过来的eup信息
	host32_t client_ip;
    uint16_t client_port;
	uint64_t task_id;

	int cmd_can_retry;
	int error_code;

}plugin_task;

void pkg_process(eservice_unit_t *eup,eservice_buf_t *recv_data);
void fill_rsp_head(QMF_PROTOCAL::QmfHead *client_head,plugin_respone_head_t *response_head,int error_code);
int plugin_pkg_error_data(plugin_task *tsp,QMF_PROTOCAL::QmfHead &client_head, const int error_code);

#endif
