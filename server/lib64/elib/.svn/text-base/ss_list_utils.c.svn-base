#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "ss_list_utils"
#endif

#include	"ss_list_utils.h"

extern uint32_t g_log_level;
extern CRollLog *g_albert_logp;			/* extern log */

int
ss_list_init_head_struct(ss_list_head_struct *hsp)
{
	if (!hsp)
		return -1;
	
	DBL_INIT_LIST_HEAD(&(hsp->head));
	hsp->number = 0;

	return 0;
}

int
ss_list_push_head_struct(ss_list_head_struct *hsp, dbl_list_head *list)
{
	if (!hsp || !list)
		return -1;

	DBL_LIST_ADDT(list, &(hsp->head));
	hsp->number++;
	
	return 0;
}

struct dbl_list_head *
ss_list_pop_head_struct(ss_list_head_struct *hsp)
{
	dbl_list_head *ptr = NULL;
	
	if (!hsp) {
		return NULL;
	}

	ptr = DBL_LIST_FIRST(&(hsp->head));
  	if (ptr) {
		DBL_LIST_DEL(ptr);
		hsp->number--;
  	}
	
	return ptr;
}

int
ss_list_del_head_struct(ss_list_head_struct *hsp, dbl_list_head *list)
{
	if (!hsp || !list) {
		return -1;
	}

	DBL_LIST_DEL(list);
	if (hsp->number == 0) {
		g_warn("number broken: %p", hsp);
	}
	else {
		hsp->number--;
	}
	
	return 0;
}

uint32_t
ss_list_info_head_struct(ss_list_head_struct *headp)
{
	if (!headp)
		return 0;
	
	return (headp->number);
}


