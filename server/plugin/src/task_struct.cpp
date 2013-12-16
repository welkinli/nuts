#include	"eservice_user.h"
#include	"common_albert.h"
#include	"program_utils.h"
#include "qmf_protocal_inter.h"
#include "plugin.h"

#define	TASK_POS_FREE	0
#define	TASK_POS_NOT_IN_LIST	1
#define	TASK_POS_WAITING	2


extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;

//逻辑数据结构
static plugin_task *g_plugin_task_chunk;
static ss_list_head_struct g_pugin_task_struct_head; //空闲队列
static ss_list_head_struct g_task_struct_inuse_list; //使用中队列

static  uint32_t g_task_seq;


static network32_t eth1_ip = 0;

static std::map<uint64_t, void *> g_task_async_map;




int plugin_init_task_struct_list(uint32_t number, uint64_t *min_val,
		uint64_t *max_val)
{
	uint32_t i;
	plugin_task *tsp = NULL;
	plugin_task *task_struct_chunk = NULL; /* task struct */
	ss_list_head_struct *headp = &g_pugin_task_struct_head;

	dlmalloc_dbg(g_plugin_task_chunk, number * sizeof(plugin_task),
			plugin_task*);
	task_struct_chunk=g_plugin_task_chunk;
	if (!g_plugin_task_chunk)
	{
		g_warn("malloc() for task struct chunk failed");
		return -2;
	}

	g_msg("task struct linker: DBLIST WITHOUT LOCK");

	ss_list_init_head_struct(headp);
	g_task_seq = (uint32_t) random();
	if (g_task_seq == 0)
		g_task_seq = 1;

	for (i = 0; i < number; i++)
	{
		tsp = (plugin_task *) (&(task_struct_chunk[i]));
		if (!tsp)
		{
			g_warn("g_mem_chunk_alloc() for task struct failed");
			return -3;
		}

		DBL_INIT_LIST_HEAD(&(tsp->list));
		ss_list_push_head_struct(headp, &(tsp->list));
		tsp->list_position = TASK_POS_FREE;

		if (i == 0)
		{
			set_pval(min_val, (uint64_t )tsp);
		}

		if (i == number - 1)
			set_pval(max_val, (uint64_t )tsp);
	}

	ss_list_init_head_struct(&g_task_struct_inuse_list);
	return 0;
}

static plugin_task *
plugin_alloc_task_struct(void)
{
	plugin_task *tsp = NULL;
	struct dbl_list_head *ptr = NULL;

	ptr = ss_list_pop_head_struct(&g_pugin_task_struct_head);
	if (!ptr)
		return NULL;

	tsp = DBL_LIST_ENTRY(ptr, plugin_task, list);

	memset(tsp, 0, sizeof(plugin_task)); /* FIXME ugly ! */

	tsp->tsp_seq = g_task_seq;
	g_task_seq++;
	if (g_task_seq == 0)
		g_task_seq = 1;

	ss_list_init_head_struct(&(tsp->output_ebp_list));

	tsp->use_time = g_current_time;
	tsp->list_position = TASK_POS_NOT_IN_LIST;
	tsp->result = -1;
	tsp->need_not_rsp = 0;
	tsp->error_code = 0;
	tsp->cmd_can_retry = 0;
	tsp->client_ip = 0;
	tsp->client_port = 0;
	g_dbg("### alloc_task_struct(), address: %p ###", tsp);

	return tsp;
}
void plugin_free_task_struct(wns_acc_task_struct *tsp)
{
	uint32_t i;
	struct dbl_list_head *ptr = NULL, *n = NULL;
	struct eservice_buf_t *ebp = NULL;

	if (!tsp)
		return;

	switch (tsp->list_position)
	{
	case TASK_POS_NOT_IN_LIST:
		break;

	case TASK_POS_WAITING:
		g_msg("free waiting tsp: %p", tsp);
		ss_list_del_head_struct(&g_task_struct_inuse_list, &(tsp->list));
		break;

	case TASK_POS_FREE:
		g_warn("want free an already freed task struct: %p", tsp);
		ss_list_del_head_struct(&g_pugin_task_struct_head, &(tsp->list));
		return;

	default:
		g_err("invalid list pos: %d", tsp->list_position);
		break;
	}

	if (tsp->already_send_respone == 0)
	{
		//g_warn("this tsp not reponse: %p", tsp);
	}

	tsp->use_time = 0;

	/* free output */DBL_LIST_FOR_EACH_SAFE(ptr, n, &(tsp->output_ebp_list.head))
	{
		if (!ptr)
			break;

		ebp = eservice_buf_list2buf(ptr);
		ESERVICE_DESTROY_EBP(ebp);
	}
	ss_list_init_head_struct(&(tsp->output_ebp_list));

	/* free input */
	ESERVICE_DESTROY_EBP(tsp->input_ebp);

	DBL_INIT_LIST_HEAD(&(tsp->list));
	ss_list_push_head_struct(&(g_pugin_task_struct_head), &(tsp->list));
	tsp->list_position = TASK_POS_FREE;

	delete_dbg(tsp->client_info_p);
	tsp->need_not_rsp = 0;

	g_dbg("### free_task_struct(), address: %p ###", tsp);

	return;
}

void plugin_add_waiting_task_struct(wns_acc_task_struct *tsp)
{
	uint32_t i;

	if (!tsp)
		return;

	if (tsp->list_position != TASK_POS_NOT_IN_LIST)
	{
		g_warn("invalid pos when add waiting: %d", tsp->list_position);
		return;
	}

	DBL_INIT_LIST_HEAD(&(tsp->list));
	ss_list_push_head_struct(&(g_task_struct_inuse_list), &(tsp->list));
	tsp->list_position = TASK_POS_WAITING;

	return;
}

void plugin_del_waiting_task_struct(wns_acc_task_struct *tsp)
{
	if (!tsp)
		return;

	if (tsp->list_position != TASK_POS_WAITING)
	{
		g_warn("invalid pos when add waiting: %d", tsp->list_position);
		return;
	}

	ss_list_del_head_struct(&g_task_struct_inuse_list, &(tsp->list));
	tsp->list_position = TASK_POS_NOT_IN_LIST;

	return;
}

int get_processing_task(uint64_t task_id,plugin_task **tspp)
{
	std::map<uint64_t, void *>::iterator the_iter = g_task_async_map.find(
			task_id);
		if (the_iter != g_task_async_map.end() && the_iter->second != NULL)
		{
			*tspp = (wns_acc_task_struct *) (the_iter->second);
			g_dbg("async tsp load, %lu -> %p, eup: %p", the_iter->first,
					the_iter->second, *eupp);
			g_task_async_map.erase(the_iter);
			plugin_del_waiting_task_struct(*tspp);
			return 0;
		}
		else
		{
			g_warn("invalid async, not found: %lu", task_id);
			return -1;
		}
}



