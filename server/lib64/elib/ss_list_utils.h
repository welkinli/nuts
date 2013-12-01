#ifndef	__SS_LIST_UTILS_H_
#define	__SS_LIST_UTILS_H_	1

#include	"common_albert.h"
#include	"albert_log.h"
#include	"dbllist.h"

typedef	struct {
	DBL_LIST_HEAD_STRUCT(head);
	uint32_t number;
} ss_list_head_struct;

#ifdef	__cplusplus
extern "C" {
#endif

int ss_list_init_head_struct(ss_list_head_struct *hsp);
int ss_list_push_head_struct(ss_list_head_struct *hsp, dbl_list_head *list);
struct dbl_list_head *ss_list_pop_head_struct(ss_list_head_struct *hsp);
int ss_list_del_head_struct(ss_list_head_struct *hsp, dbl_list_head *list);
uint32_t ss_list_info_head_struct(ss_list_head_struct *headp);

#ifdef	__cplusplus
}
#endif

#endif

