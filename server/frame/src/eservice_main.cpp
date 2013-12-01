#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "ESERVICE_MAIN"
#endif

#include	"eservice_main.h"
#include	"eservice_network.h"
#include	"eservice_worker.h"
#include	<malloc.h>

CRollLog *g_albert_logp = NULL;
uint32_t g_log_level = G_ALOG_LEVEL_DBG;
uint32_t g_max_connection_num;
struct timeval g_tcp_conn_timeout;
struct timeval g_event_timeout;
time_t g_current_time;					/* current time, seconds from 1970-1-1 */
struct timeval g_current_tv;

CRollLog *g_es_frame_logp = NULL;
uint32_t g_es_frame_log_level = G_ALOG_LEVEL_DBG;

CRollLog *g_es_user_plugin_logp = NULL;
uint32_t g_es_user_plugin_log_level = G_ALOG_LEVEL_DBG;

const char *g_es_version = "v0.1";
eservice_main_conf_t g_es_main_conf;
char *g_es_main_conf_path = NULL;

uint32_t need_reopen = 0;
uint32_t now_terminate = 0;
uint32_t g_acceptfd_lowlimit = 0;
eservice_manager_t g_es_manager;


static void 
__sig_hup_func(int signo)
{
	need_reopen = 1;
	return;
}

static void
__sig_int_func(int signo)
{
	now_terminate = 1;
	return;
}

static int
__prepare_child_process(eservice_main_conf_t *confp)
{
	int res;
	uint32_t i;

	g_msg("fork() %d processes", confp->worker_num);
	
	for (i = 0; i < confp->worker_num; ++i) {
		res = fork();
		if (res < 0) {
			g_warn("fork() failed: %s", strerror(errno));
			exit(1);
		}
		
		if (res == 0) {
			g_msg("fork a child %d, pid: %d", i + 1, getpid());
			return i + 1;
            	}
	}

	return 0;
}

static int
__load_user_mod(eservice_manager_t *mgr, const char *so_path, const char *so_arg)
{
	int res = -255;
	
	g_msg("init service module: %s, %s", so_path, so_arg);

	if (!so_path)
		return eservice_errcode_param_invalid;
	
	memset(mgr, 0, sizeof(*mgr));

	mgr->module_path = strdup(so_path);
	mgr->module = g_module_open_albert(mgr->module_path);
	if (!mgr->module) {
		g_warn("load module \"%s\" failed: %s", mgr->module_path, g_module_error_albert());
		return -2;
	}
		
	g_msg("load module \"%s\" success", mgr->module_path);
		
	/* init */
	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_INIT_NAME, (void * *)&(mgr->cbs.mod_init.user_callback_func)) < 0) {
		g_warn("%s", g_module_error_albert());
		return -3;
	}

	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_DATA_ARRIVE_NAME, (void * *)&(mgr->cbs.data_arrive)) < 0) {
		g_warn("no "ESERVICE_CB_DATA_ARRIVE_NAME" callback");
		return -3;
	}

	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_PRE_RUN_NAME, (void * *)&(mgr->cbs.pre_run.user_callback_func)) < 0) {
		g_msg("no "ESERVICE_CB_PRE_RUN_NAME" callback");
	}

	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_AFTER_ACCEPT_NAME, (void * *)&(mgr->cbs.after_accept.user_callback_func)) < 0) {
		g_msg("no "ESERVICE_CB_AFTER_ACCEPT_NAME" callback");
	}

	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_LISTEN_TIMEOUT_NAME, (void * *)&(mgr->cbs.listen_timeout.user_callback_func)) < 0) {
		g_msg("no "ESERVICE_CB_LISTEN_TIMEOUT_NAME" callback");
	}

	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_TCP_CONN_TIMEOUT_NAME, (void * *)&(mgr->cbs.tcp_conn_timeout.user_callback_func)) < 0) {
		g_msg("no "ESERVICE_CB_TCP_CONN_TIMEOUT_NAME" callback");
	}

	if (g_module_symbol_albert (mgr->module, ESERVICE_CB_TCP_TERMINATE_NAME, (void * *)&(mgr->cbs.tcp_terminate)) < 0) {
		g_msg("no "ESERVICE_CB_TCP_TERMINATE_NAME" callback");
	}

	mgr->garg = (void *)strdup(so_arg);
	
	return 0;
}


int
main(int argc, char **argv)
{
	int res;
	uint32_t i;
	char tmp_buf[MAX_LINE_LEN];
	char proto_buf[MAX_CONF_LEN];
	char port_buf[MAX_CONF_LEN];
	char ip_buf[MAX_CONF_LEN];
	char device_buf[MAX_CONF_LEN];
	
	//ProfilerStart("./profile_task");

	/* init thread */
/*
#if defined G_THREADS_ENABLED && defined G_THREADS_IMPL_POSIX
	g_dbg("Hello World posix thread !\n");
	g_thread_init (NULL);
#else
	printf("no thread implementation !\n");
	exit(0);
#endif
*/

	//mallopt(M_MMAP_MAX, 0); // 禁止malloc调用mmap分配内存
	//mallopt(M_TRIM_THRESHOLD, -1); // 禁止内存紧缩
	
	if (argc != 2) {
		g_warn("usage: %s <eservice main config path>\n", argv[0]);
		exit(1);
	}
	
	g_es_main_conf_path = argv[1];
	memset(&g_es_main_conf, 0, sizeof(g_es_main_conf));
	
	/* set seed */
	srand(getpid() + time(NULL));

	/* read main conf */
	Misc_Cfg_GetConfig(g_es_main_conf_path,
		"USE_DAEMON", CFG_INT, &(g_es_main_conf.use_daemon), 1,
		"LOG_LEVEL", CFG_INT, &(g_es_main_conf.log_level), G_ALOG_LEVEL_DBG,
		"MAX_CONN_NUM", CFG_INT, &(g_es_main_conf.max_conn_num), 800,
		"LISTEN_TIMEOUT", CFG_INT, &(g_es_main_conf.listen_timeout), 1000,
		"CONN_TIMEOUT", CFG_INT, &(g_es_main_conf.conn_timeout), 1000,
		
		"GLORY_SIZE", CFG_INT, &(g_es_main_conf.glory_size), 10,
		"NETWORK_LOG_PATH", CFG_STRING, g_es_main_conf.network_log_path, "../log/c.log", sizeof(g_es_main_conf.network_log_path),

		"BIND_GROUP_NUM", CFG_INT, &(g_es_main_conf.bind_group_num), 1,
		"WORKER_NUM", CFG_INT, &(g_es_main_conf.worker_num), 1,

		"LOGIC_SO_PATH", CFG_STRING, g_es_main_conf.logic_so_path, "../lib/no_such_mod", sizeof(g_es_main_conf.logic_so_path),
		"LOGIC_SO_ARG", CFG_STRING, g_es_main_conf.logic_so_arg, "../lib/no_such_arg", sizeof(g_es_main_conf.logic_so_arg),
		"LOGIC_SO_LOG_PATH", CFG_STRING, g_es_main_conf.logic_so_log, "", sizeof(g_es_main_conf.logic_so_log),
		"LOGIC_SO_LOG_LEVEL", CFG_INT, &(g_es_main_conf.logic_so_log_level), G_ALOG_LEVEL_DBG,

		"PROCESS_NUM", CFG_INT, &(g_es_main_conf.process_num), 1,
		"ACCEPT_LIMIT",CFG_INT, &(g_es_main_conf.acceptfd_limit), 100000,
		"ACCEPT_LOW_LIMIT_PER",CFG_INT, &(g_es_main_conf.acceptfd_low_limit_per), 98,
		NULL
        );

	g_acceptfd_lowlimit = g_es_main_conf.acceptfd_limit * g_es_main_conf.acceptfd_low_limit_per / 100 ;
	/* log level */
	g_log_level = g_es_main_conf.log_level;
	/*max connection num*/
	g_max_connection_num = g_es_main_conf.acceptfd_limit * g_es_main_conf.process_num;
	/* timeout */
	if (g_es_main_conf.listen_timeout > 0) {
		msec2timeval(&g_event_timeout, g_es_main_conf.listen_timeout);
	}
	
	if (g_es_main_conf.conn_timeout > 0) {
		msec2timeval(&g_tcp_conn_timeout, g_es_main_conf.conn_timeout);
	}

	/* bind group */
	if (g_es_main_conf.bind_group_num > ESERVICE_MAX_BIND_GROUP) {
		g_warn("too many bind group: %u", g_es_main_conf.bind_group_num);
		exit(1);
	}

	for (i = 0; i < g_es_main_conf.bind_group_num; i++) {
		snprintf(proto_buf, sizeof(proto_buf) - 1, "BIND_GROUP_%u_PROTO", i + 1);
		snprintf(port_buf, sizeof(port_buf) - 1, "BIND_GROUP_%u_PORT", i + 1);
		snprintf(ip_buf, sizeof(ip_buf) - 1, "BIND_GROUP_%u_ADDRESS", i + 1);
		snprintf(device_buf, sizeof(device_buf) - 1, "BIND_GROUP_%u_DEVICE", i + 1);

		Misc_Cfg_GetConfig(g_es_main_conf_path,
			proto_buf, CFG_STRING,tmp_buf, "tcp", sizeof(tmp_buf),
			port_buf, CFG_STRING, g_es_main_conf.bind_group[i].port_str, "", sizeof(g_es_main_conf.bind_group[i].port_str),
			ip_buf, CFG_STRING, g_es_main_conf.bind_group[i].ip, "", sizeof(g_es_main_conf.bind_group[i].ip),
			device_buf, CFG_STRING, g_es_main_conf.bind_group[i].device, "", sizeof(g_es_main_conf.bind_group[i].device),

			NULL
	        );

		if (strlen(g_es_main_conf.bind_group[i].device) >= 2 && strlen(g_es_main_conf.bind_group[i].ip) >= 4) {
			g_warn("cannot combine device and ip, device: %s, ip: %s", g_es_main_conf.bind_group[i].device, g_es_main_conf.bind_group[i].ip);
			exit(1);
		}

		if (!strncasecmp(tmp_buf, "tcp", 3)) {
			g_es_main_conf.bind_group[i].proto_type = SOCK_STREAM;
		}
		else if (!strncasecmp(tmp_buf, "udp", 3)) {
			g_es_main_conf.bind_group[i].proto_type = SOCK_DGRAM;
		}
		else {
			g_warn("unknown proto: %s", tmp_buf);
			exit(1);
		}
	}
	
	/* worker */
	if (g_es_main_conf.worker_num > ESERVICE_MAX_WORKER_NUM) {
		g_warn("too many worker: %u", g_es_main_conf.worker_num);
		exit(1);
	}

	for (i = 0; i < g_es_main_conf.worker_num; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf) - 1, "WORKER_CONF_PATH_%u", i + 1);

		Misc_Cfg_GetConfig(g_es_main_conf_path,
			tmp_buf, CFG_STRING, g_es_main_conf.worker_conf_pathp[i], "../conf/w.conf", sizeof(g_es_main_conf.worker_conf_pathp[i]),

			NULL
	        );
	}

	if (g_es_main_conf.use_daemon) {
		daemon(1, 1);
	}

	/* open frame log */
	g_es_frame_log_level = g_es_main_conf.log_level;
	g_es_frame_logp = new CRollLog();
	if (!g_es_frame_logp) {
		g_err("init frame log failed");
	}

	g_es_frame_logp->init(string(g_es_main_conf.network_log_path), "", 1, 20*(1<<20), 10);				/* micro second */
	//g_es_frame_logp->init(string(g_es_main_conf.network_log_path), "", 0, 20*(1<<20), 10);				/* second */

	
	/* open plugin log */
	if (strlen(g_es_main_conf.logic_so_log) <= 1) {
		g_dbg("no user plugin log, use the frame log");
		g_es_user_plugin_log_level = g_es_frame_log_level;
		g_es_user_plugin_logp = g_es_frame_logp;
	}
	else {
		g_es_user_plugin_log_level = g_es_main_conf.logic_so_log_level;
		g_es_user_plugin_logp = new CRollLog();
		if (!g_es_user_plugin_logp) {
			g_err("init plugin log failed");
		}

		g_es_user_plugin_logp->init(string(g_es_main_conf.logic_so_log), "", 1, 20*(1<<20), 10);				/* micro second */
		//g_es_user_plugin_logp->init(string(g_es_main_conf.logic_so_log), "", 0, 20*(1<<20), 10);				/* second */

		ASSIGN_PLUGIN_LOG(g_albert_logp, g_log_level);
		g_warn("========== plugin log start ==========\n");
	}

	ASSIGN_FRAME_LOG(g_albert_logp, g_log_level);	
	g_warn("========== frame log start ==========\n");
	
	/* time */
	time(&g_current_time);	
	gettimeofday(&g_current_tv, NULL);

	/* signal */
	signal(SIGHUP, __sig_hup_func);
	signal(SIGINT, __sig_int_func);
	signal(SIGTERM, __sig_int_func);

	/* load user mod */
	res = __load_user_mod(&g_es_manager, g_es_main_conf.logic_so_path, g_es_main_conf.logic_so_arg);
	if (res < 0) {
		g_warn("__load_user_mod() failed: %d", res);
		exit(1);
	}
	
#if 0		
	res = __prepare_child_process(&g_es_main_conf);
	if (res < 0) {
		g_err("__prepare_child_process() failed: %d", res);
	}
	else if (res == 0) {				/* parent */
		eservice_network_run(&g_es_main_conf);
	}
	else {						/* child */
		eservice_worker_run(&g_es_main_conf, res);
	}
#else
	eservice_network_run(&g_es_manager, &g_es_main_conf);
#endif

	return 0;
}

