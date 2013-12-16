/*
 * user.h
 *
 *  Created on: 2013-12-8
 */

#ifndef USER_H_
#define USER_H_


int plugin_init_user_list(uint32_t number, uint64_t *min_val, uint64_t *max_val);
group_info_t *alloc_user(uint64_t uin, uint64_t key, int section_id);

void reset_user(user_info_t *p_user);

int get_group_users(int group_id, char **uins);
int get_group_users(uint64_t uin, uint64_t **uins, int *uin_count);
int add_user_into_map(user_info_t *puser);
int remove_user_from_map(uint64_t uin);
int get_user_from_map(uint64_t uin, user_info_t **pp_user);
int join_group(user_info_t *p_user);
int leave_group(user_info_t *p_user);


#endif /* USER_H_ */
