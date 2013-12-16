#ifndef _task_struck_h_shazi
#define _task_struck_h_shazi
#include	"eservice_user.h"
#include	"common_albert.h"
#include	"program_utils.h"
#include "qmf_protocal_inter.h"
#include "plugin.h"

#define	TASK_POS_FREE	0
#define	TASK_POS_NOT_IN_LIST	1
#define	TASK_POS_WAITING	2


int plugin_init_task_struct_list(uint32_t number, uint64_t *min_val,uint64_t *max_val);

plugin_task *plugin_alloc_task_struct(void);

void plugin_free_task_struct(plugin_task *tsp);

void plugin_add_waiting_task_struct(plugin_task *tsp);


void plugin_del_waiting_task_struct(plugin_task *tsp);

int get_processing_task(uint64_t task_id,plugin_task **tspp);



#endif


