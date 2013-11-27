#ifndef	_ESERVICE_NETWORK_albert_
#define	_ESERVICE_NETWORK_albert_	1

#include	"eservice_main.h"

#define	ES_CONTEXT_BUF_DEFAULT_LEN	2048
#define	ES_CONTEXT_DATA_IMM_LEN	1500
#define	ES_DEFAULT_WRITE_TIMEOUT	10
#define	ES_TCP_MAX_ACCEPT_ONCE	20
#define	ES_TCP_RECVBUF_SIZE	(4096)
#define	ES_TCP_SENDBUF_SIZE	(4096)
#define	ES_TCP_RECONN_TIME		1
#define	ES_TCP_PROCESS_SPE_LISTEN_PORT_BASE	27000
#define	ES_TCP_MAX_QUEUE_WRITE	20
#define	ES_TCP_MAX_LISTEN_PORT	256

#define	ES_UPDATE_CONN_ACTIVE(s, t)	((s)->last_active = (t))

#define	ES_INVALID_CONN(s)	(!(s) || (s)->status == es_context_status_unused)
#define	ES_INVALID_CONN_RET(s, r)	do {if (ES_INVALID_CONN(s)) {g_warn("invalid sp: %p", (s)); return (r);}} while (0)

#define	ES_INVALID_EUP(e)	(!(e) || (e)->used == 0 || !((e)->sp) || (e)->seq != (e)->sp->seq || ES_INVALID_CONN((e)->sp) || (uint64_t)((e)->sp->linked_eup) != (uint64_t)(e))
#define	ES_INVALID_EUP_RET(e, r)	do {if (ES_INVALID_EUP(e)) {g_warn("the connection is not valid any longer: %p", (e)); return (r);}} while (0)

enum {
	es_context_status_unused = 0,
	es_context_status_used = 1,
};

enum {
	es_socket_type_tcp_listen = 1,
	es_socket_type_tcp_conn = 2,
	es_socket_type_tcp_client = 3,
	es_socket_type_tcp_client_once = 4,
	es_socket_type_udp = 5,
};

enum {
	es_high_priority = 0,
	es_middle_priority = 1,
	es_low_priority = 2,
};

typedef struct _avaiable_head
{
	uint32_t	total_num;
	struct dbl_list_head head;
} es_avaiable_head;


typedef	struct {
	int reconn_count;					/* not used */
	int reconn_ms;
	int conn_in_progress;
	struct event *reconn_timerev;
	ssize_t last_written_data_len;
	ssize_t last_last_written_data_len;
} eservice_reconn_t;

typedef	struct {
	int close_after_next_write;
	ssize_t want_len;
	struct eservice_buf_t *buf;
} eservice_reader_t;

typedef	struct {
	struct dbl_list_head sending_list_header;
	uint32_t n_to_write;
} eservice_writer_t;

typedef	struct {
	eservice_cb_struct tcp_conn_timeout;
	eservice_cb_struct tcp_terminate;
	eservice_cb_struct data_arrive;
} eservice_conn_cbs_t;

struct eservice_unit_t;

struct LRU_node{
	struct dbl_list_head list;
	uint64_t *sp;
};

typedef struct {
	struct dbl_list_head list;
	uint32_t seq;
	int fd;
	uint32_t related_port;				/* the connection related listen port or udp port */
	struct sockaddr_in related_native;
	int type;
	struct sockaddr_in peer;
	struct timeval to;
	eservice_manager_t *egr;
	struct event *ev;
	struct event_base *ebase;
	int status;
	struct eservice_unit_t *linked_eup;
	time_t use_time;
	time_t last_active;
	short last_event;
	
	/* reconn control */
	eservice_reconn_t reconn_control;

	/* IO control */
	eservice_reader_t pending_read_control;
	eservice_writer_t waiting_write_control;

	/* per connect cbs */
	eservice_conn_cbs_t per_conn_cbs;

	/* LRU_node */
	struct LRU_node lru_node;
	/* cdn client ip */
	/*uint32_t proxy_cdn_client_ip;*/

	/* user data */
	void *user_data;
}es_conn_context;

struct eservice_unit_t {
	struct dbl_list_head list;
	uint32_t used;
	uint32_t seq;
	es_conn_context *sp;
};

typedef	struct {
	uint32_t used;
	uint32_t port;
	uint32_t recv_num;
} port_statistic_t;


extern int es_reconnect_tcp_context(es_conn_context *sp);
extern int es_timer_reconnect(es_conn_context *sp);
extern es_conn_context *es_create_tcp_conn(struct sockaddr_in *sa, int reconn_ms, struct timeval *to);
extern es_conn_context *es_create_tcp_imme(struct sockaddr_in *sa, int reconn_ms, struct timeval *to);
extern int es_destroy_tcp_conn(es_conn_context *sp);
extern int es_conn_context_write(es_conn_context *sp);
extern int eservice_network_run(eservice_manager_t *mgr, eservice_main_conf_t *confp);

extern void es_assign_plugin_log(void);
extern void es_assign_frame_log(void);

#endif

