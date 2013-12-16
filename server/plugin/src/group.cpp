/*
 * group.cpp
 *
 */

#include "group.h"
#include "plugin.h"

//各个区有各个区的进程
group_info_t *g_group_data_chunk;
ss_list_head_struct g_group_info_head; //空闲队列
ss_list_head_struct g_group_info_inuse_list; //使用中队列

uint32_t g_group_id;

#define user_plus_plus(puser,new_group) do{\
	puser->pgroup->now_user_count++;\
	puser->pgroup->psection->now_user_count++;\
	} while(0);


int plugin_init_group_list(uint32_t number, uint64_t *min_val,
		uint64_t *max_val)
{
	uint32_t i;
	group_info_t *p_group = NULL;
	group_info_t *task_struct_chunk = NULL; /* task struct */
	ss_list_head_struct *headp = &g_group_info_head;

	dlmalloc_dbg(g_group_data_chunk, number * sizeof(group_info_t),
			group_info_t*);
	task_struct_chunk = g_group_data_chunk;
	if (!g_group_data_chunk)
	{
		g_warn("malloc() for group struct chunk failed");
		return -2;
	}

	g_msg("group struct linker: DBLIST WITHOUT LOCK");

	ss_list_init_head_struct(headp);
	g_group_id = 1;

	for (i = 0; i < number; i++)
	{
		p_group = (group_info_t *) (&(task_struct_chunk[i]));
		if (!p_group)
		{
			g_warn("g_mem_chunk_alloc() for group struct failed");
			return -3;
		}

		DBL_INIT_LIST_HEAD(&(p_group->list));
		ss_list_push_head_struct(headp, &(p_group->list));

		if (i == 0)
		{
			set_pval(min_val, (uint64_t )p_group);
		}

		if (i == number - 1)
			set_pval(max_val, (uint64_t )p_group);
	}

	ss_list_init_head_struct(&g_group_info_inuse_list);
	return 0;
}

group_info_t *alloc_group(int section_id)
{
	group_info_t *p_group = NULL;
	struct dbl_list_head *ptr = NULL;

	ptr = ss_list_pop_head_struct(&g_group_info_head);
	if (!ptr)
		return NULL;

	p_group = DBL_LIST_ENTRY(ptr, group_info_t, list);

	memset(p_group, 0, sizeof(group_info_t)); /* FIXME ugly ! */

	p_group->group_id = g_group_id;
	g_group_id++;
	section_info_t *section_config = (section_info_t *) (&g_section[section_id]);
	p_group->psection = section_config;
	p_group->create_time = g_current_time;
	p_group->section_id = section_config->section_id; //大区id
	memset(p_group->uins, 0, sizeof(p_group->uins));
	p_group->init_bet_count = section_config->init_bet_count; //初始下注大小
	p_group->min_bet_count = section_config->min_bet_count; //跟注下限
	p_group->max_bet_count = section_config->max_bet_count; //跟注上限

	g_dbg("### alloc_group_struct(), address: %p ###", p_group);

	ss_list_push_head_struct(&g_group_info_inuse_list, &(p_group->list));
	return p_group;
}


void reset_group(group_info_t *p_group)
{
	uint32_t i;
	struct dbl_list_head *ptr = NULL, *n = NULL;

	if (!p_group)
		return;
	ss_list_del_head_struct(&g_group_info_inuse_list, &(p_group->list));
	ss_list_push_head_struct(&(g_group_info_head), &(p_group->list));

	((section_info_t *)p_group->psection)->now_group_count--;
	memset(p_group, 0, sizeof(group_info_t));

	return;
}
