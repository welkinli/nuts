#ifndef	_ESERVICE_WORKER_albert_
#define	_ESERVICE_WORKER_albert_	1

#include	"eservice_main.h"

typedef	struct {
	char name[MAX_CONF_LEN];
	char log_path[MAX_CONF_LEN];
} eservice_worker_conf_t;

extern int eservice_worker_run(eservice_main_conf_t *confp,  int worker_id);

#endif
