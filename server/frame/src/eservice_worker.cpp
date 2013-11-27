#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "ESERVICE_WORKER"
#endif

#include	"eservice_worker.h"

/* global extern */
extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;
extern time_t g_current_time;
extern struct timeval g_current_tv;
extern uint32_t now_terminate;

/* global define */
eservice_worker_conf_t g_es_worker_conf;

int
eservice_worker_run(eservice_main_conf_t *confp, int worker_id)
{
	int res = -255;
	char name[MAX_CONF_LEN];
	char log_path[MAX_CONF_LEN];
	
	memset(&g_es_worker_conf, 0, sizeof(g_es_worker_conf));

	snprintf(name, sizeof(name) - 1, "worker_%d", worker_id);
	snprintf(log_path, sizeof(log_path) - 1, "../log/%s", name);
	
	/* read worker conf */
	Misc_Cfg_GetConfig(confp->worker_conf_pathp[worker_id - 1],
		"NAME", CFG_STRING, g_es_worker_conf.name, name, sizeof(g_es_worker_conf.name),
		"LOG_PATH", CFG_STRING, g_es_worker_conf.log_path, log_path, sizeof(g_es_worker_conf.log_path),
		
		NULL
        );
	
	/* open log */
	
	while (1) {
		if (now_terminate > 0) {
			g_warn("now terminated");
			exit(0);
		}
		
		sleep(1);
		g_dbg("%s tick", g_es_worker_conf.name);
	}
	
	return 0;
}

