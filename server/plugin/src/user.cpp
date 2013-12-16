/*
 * user.cpp
 *
 */

#include "group.h"
#include "plugin.h"

//保持到server退出的时候才释放内存，中间不需要考虑内存释放的情况
group_info_t *g_user_data_chunk;
ss_list_head_struct g_user_info_head; //空闲队列

//设定一个timer，每秒钟读inuser_list 并判断状态status在使用中， 往数据库和cache中更新数据,不用在游戏中每次操作都更新内存和数据库
//有可能用户刚加入inuse队列但是，money数据还未获取到
//只有在用户进来，并分配到inuser_list的时候,才去取一次cache中的money数据

ss_list_head_struct g_user_info_inuse_list; //使用中队列

//可以用于掉线重连的情况下找回自己的位置和之前保存的数据,当然这种一般都清掉了
std::map<uint64_t, void *> g_uin2puser_map;

static enum
{
	WAITING = 0, PLAYING = 1, IDLE = 2,
};
#define user_plus_plus(puser,plus_group) do{\
	puser->pgroup->now_user_count++;\
	puser->pgroup->psection->now_user_count++;\
	if(plus_group) \
	puser->pgroup->psection->now_group_count++;\
	} while(0);

#define user_minus_minus(puser,minus_group) do{\
	puser->pgroup->now_user_count--;\
	puser->pgroup->psection->now_user_count--;\
	if(minus_group) \
	puser->pgroup->psection->now_group_count--;\
	} while(0);

//设置用户已经开始进入牌局了
#define set_puser_inuse(puser) puser->status=1

int plugin_init_user_list(uint32_t number, uint64_t *min_val, uint64_t *max_val)
{
	uint32_t i;
	user_info_t *p_user = NULL;
	user_info_t *task_struct_chunk = NULL; /* task struct */
	ss_list_head_struct *headp = &g_user_info_head;

	dlmalloc_dbg(g_user_data_chunk, number * sizeof(user_info_t), user_info_t*);
	task_struct_chunk = g_user_data_chunk;
	if (!g_user_data_chunk)
	{
		g_warn("malloc() for user struct chunk failed");
		return -2;
	}

	g_msg("user struct linker: DBLIST WITHOUT LOCK");

	ss_list_init_head_struct(headp);

	for (i = 0; i < number; i++)
	{
		p_user = (user_info_t *) (&(task_struct_chunk[i]));
		if (!p_user)
		{
			g_warn("g_mem_chunk_alloc() for user struct failed");
			return -3;
		}

		DBL_INIT_LIST_HEAD(&(p_user->list));
		ss_list_push_head_struct(headp, &(p_user->list));

		if (i == 0)
		{
			set_pval(min_val, (uint64_t )p_user);
		}

		if (i == number - 1)
			set_pval(max_val, (uint64_t )p_user);
	}

	ss_list_init_head_struct(&g_user_info_inuse_list);
	return 0;
}

//alloc user 之后应该紧接着去获取用户的筹码情况
group_info_t *alloc_user(uint64_t uin, uint64_t key, int section_id)
{
	user_info_t *p_user = NULL;
	struct dbl_list_head *ptr = NULL;

	ptr = ss_list_pop_head_struct(&g_user_info_head);
	if (!ptr)
		return NULL;

	p_user = DBL_LIST_ENTRY(ptr, user_info_t, list);

	p_user->init();

	section_info_t *section_config = (section_info_t *) (&g_section[section_id]);

	p_user->uin = uin;
	p_user->key = key;
	p_user->create_time = g_current_time;
	p_user->section_id = section_config->section_id; //大区id
	g_uin2puser_map[uin] = p_user;

	g_dbg("### alloc_user_struct(), address: %p ###", p_user);

	ss_list_push_head_struct(&g_user_info_inuse_list, &(p_user->list));
	return p_user;
}

void reset_user(user_info_t *p_user)
{
	uint32_t i;
	struct dbl_list_head *ptr = NULL, *n = NULL;

	if (!p_user)
		return;
	ss_list_del_head_struct(&g_user_info_inuse_list, &(p_user->list));
	ss_list_push_head_struct(&(g_user_info_head), &(p_user->list));
	p_user->init();

	return;
}

int get_group_users(int group_id, char **uins)
{
	return 0;
}

int get_group_users(uint64_t uin, uint64_t **uins, int *uin_count)
{
	user_info_t *p_user = NULL;
	get_user_from_map(uin, &p_user);
	if (!p_user)
	{
		*uins = p_user->pgroup->uins;
		*uin_count = p_user->pgroup->now_user_count;
		return 0;
	}
	//TODO
	return -1;
}
int add_user_into_map(user_info_t *puser)
{
	if (!puser)
		return -1;
	g_uin2puser_map[puser->uin] = puser;
	return 0;
}

int remove_user_from_map(uint64_t uin)
{
	std::map<uint64_t, void *>::iterator the_iter = g_uin2puser_map.find(uin);
	if (the_iter != g_uin2puser_map.end() && the_iter->second != NULL)
	{
		g_uin2puser_map.erase(the_iter);
		return 0;
	}
	else
	{
		//TODO
		g_warn("g_uin2puser_map  not found: %lu", uin);
		return -1;
	}
}

int get_user_from_map(uint64_t uin, user_info_t **pp_user)
{
	std::map<uint64_t, void *>::iterator the_iter = g_uin2puser_map.find(uin);
	if (the_iter != g_uin2puser_map.end() && the_iter->second != NULL)
	{
		*pp_user = (user_info_t *) (the_iter->second);
		return 0;
	}
	else
	{
		//TODO
		*pp_user = NULL;
		g_warn("g_uin2puser_map  not found: %lu", uin);
		return -1;
	}
}

//目前是新用户分配group
//后续改进成随机分配group,以及切换group的功能
//>=0 成功 返回座位号 ，失败<0
int join_group(user_info_t *p_user)
{
	group_info_t *p_group = NULL;
	uint64_t uin = p_user->uin;
	int section_id = p_user->section_id;

	struct dbl_list_head *ptr = NULL, *next = NULL;
	DBL_LIST_FOR_EACH_SAFE(ptr, next, &(g_user_info_inuse_list.head))
	{
		if (!ptr)
			break;
		p_group = DBL_LIST_ENTRY(ptr, group_info_t, list);
		if (!p_group && p_group->now_user_count >= 0
				&& p_group->now_user_count < p_group->max_group_user_count)
		{
			for (int i = 0; i < MAX_UIN_NUM && i < p_group->now_user_count; i++)
			{
				//找到一个空位，填补uins[i]的空缺，基本上就是补到i这个座位上
				if (p_group->uins[i] != 0)
				{
					//填充自身状态
					p_group->uins[i] = uin;
					p_user->status = PLAYING;
					p_user->seat_id = i;
					p_user->group_id = p_group->group_id;
					p_user->pgroup = p_group;
					//处理计数
					user_plus_plus(p_user, false);
					//处理map
					add_user_into_map(p_user);
					return i;
				}
			}
		}
	}
	//需要新分配一个group
	p_group = alloc_group(section_id);
	if (!p_group)
		return -1;
	//填充自身状态
	p_group->uins[0] = uin;
	p_user->seat_id = 0;
	p_user->group_id = p_group->group_id;
	p_user->pgroup = p_group;
	//处理计数
	user_plus_plus(p_user, true);
	//处理map
	add_user_into_map(p_user);
	return 0;

}

//用户长时间位操作，或者掉线的情况，执行此操作

int leave_group(user_info_t *p_user)
{
	uint64_t uin = p_user->uin;
	group_info_t *p_group = p_user->pgroup;
	if (p_group != NULL && p_group->uins[p_user->seat_id] == uin)
	{
		p_group->uins[p_user->seat_id] = 0;
		//清map
		remove_user_from_map(uin);
		//处理group
		if (p_group->now_user_count <= 0)
		{
			reset_group(p_group);
			user_minus_minus(p_user,true);
		}
		else
		{
			user_minus_minus(p_user,false);
		}
		//处理自己
		p_user->init();
	}
	else
	{
		//座位号不匹配
		//TODO
		return -1;
	}
	return 0;
}

