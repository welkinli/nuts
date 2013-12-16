/*
 * group.h
 *
 *  Created on: 2013-12-7
 */

#ifndef GROUP_H_
#define GROUP_H_
#include "plugin.h"

int plugin_init_group_list(uint32_t number, uint64_t *min_val,
		uint64_t *max_val);
group_info_t *alloc_group(int section_id);

void reset_group(group_info_t *p_group);

#endif /* GROUP_H_ */
