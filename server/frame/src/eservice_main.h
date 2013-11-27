#ifndef	_ESERVICE_MAIN_albert_
#define	_ESERVICE_MAIN_albert_	1

#include	"common_albert.h"
#include	"program_utils.h"
#include	"ss_list_utils.h"
#include	"socket_utils.h"
#include	"dbllist.h"
#include	"albert_log.h"
#include	"module_albert.h"
#include	"event2/event.h"
#include	"event2/listener.h"
#include	"event2/bufferevent.h"
#include	"event2/buffer.h"
#include	"eservice_buf.h"
#include	"eservice_user.h"

#include	"misc_cfg.h"

#define	ESERVICE_MAX_WORKER_NUM	20
#define	ESERVICE_MAX_BIND_GROUP	10
#define	ESERVICE_SYSTEM_MAX_FILENO	3000000
#define	ESERVICE_PRIORITY_NUM	3

#define	ESERVICE_CB_INIT_NAME	"eservice_cb_init"
#define	ESERVICE_CB_PRE_RUN_NAME	"eservice_cb_pre_run"
#define	ESERVICE_CB_AFTER_ACCEPT_NAME	"eservice_cb_after_accept"
#define	ESERVICE_CB_LISTEN_TIMEOUT_NAME	"eservice_cb_listen_timeout"
#define	ESERVICE_CB_TCP_CONN_TIMEOUT_NAME	"eservice_cb_tcp_conn_timeout"
#define	ESERVICE_CB_TCP_TERMINATE_NAME	"eservice_cb_tcp_terminate"
#define	ESERVICE_CB_DATA_ARRIVE_NAME	"eservice_cb_data_arrive"

/* error code */
enum {
	eservice_errcode_param_invalid = -100,
	
	eservice_errcode_mm = -119,
};

typedef	struct {
	int proto_type;
	char port_str[64];
	char ip[32];
	char device[32];
} eservice_bind_group_t;

typedef	struct {
	uint32_t use_daemon;
	uint32_t log_level;
	uint32_t max_conn_num;
	int listen_timeout;
	int conn_timeout;
	char network_log_path[MAX_CONF_LEN];
	uint32_t glory_size;

	/* bind group */
	uint32_t bind_group_num;
	eservice_bind_group_t bind_group[ESERVICE_MAX_BIND_GROUP];
	
	/* worker */
	uint32_t worker_num;
	char worker_conf_pathp[ESERVICE_MAX_WORKER_NUM][MAX_CONF_LEN];

	/* user mod */
	char logic_so_path[MAX_CONF_LEN];
	char logic_so_arg[MAX_CONF_LEN];
	char logic_so_log[MAX_CONF_LEN];
	uint32_t logic_so_log_level;

	uint32_t process_num;
	uint32_t acceptfd_limit;
	uint32_t acceptfd_low_limit_per;
} eservice_main_conf_t;

struct eservice_unit_t;
struct eservice_manager_t;

typedef eservice_callback_return_val (*eservice_user_process_callback)(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg);

typedef struct {
	eservice_user_process_callback user_callback_func;
	void *earg;
} eservice_cb_struct;

typedef struct {
	eservice_cb_struct mod_init;
	eservice_cb_struct pre_run;
	eservice_cb_struct after_accept;
	eservice_cb_struct listen_timeout;
	eservice_cb_struct tcp_conn_timeout;
	eservice_cb_struct tcp_terminate;
	eservice_cb_struct data_arrive;
} eservice_callback_functions;

typedef	struct eservice_manager_t {
	struct event_base *ebase;
	GModule_albert_t *module;
	char *module_path;
	void *garg;
	eservice_callback_functions cbs;
} eservice_manager_t;

#define	ASSIGN_PLUGIN_LOG(log, level)	do {(log) = g_es_user_plugin_logp; (level) = g_es_user_plugin_log_level;} while (0)
#define	ASSIGN_FRAME_LOG(log, level)	do {(log) = g_es_frame_logp; (level) = g_es_frame_log_level;} while (0)

#define	CALL_USER_PLUGIN(r, f, arg1, arg2, arg3, arg4)		do {\
															ASSIGN_PLUGIN_LOG(g_albert_logp, g_log_level);\
															(r) = (f)((arg1), (arg2), (arg3), (arg4));\
															ASSIGN_FRAME_LOG(g_albert_logp, g_log_level);\
														} while (0)

#define	CALL_USER_BUF_PLUGIN(r, f, arg1, arg2, arg3)		do {\
															ASSIGN_PLUGIN_LOG(g_albert_logp, g_log_level);\
															(r) = (f)((arg1), (arg2), (arg3));\
															ASSIGN_FRAME_LOG(g_albert_logp, g_log_level);\
														} while (0)

#endif	/* !_ESERVICE_MAIN_albert_ */

