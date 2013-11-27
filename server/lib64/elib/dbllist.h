
#ifndef _DBLLISTS_H
#define _DBLLISTS_H



#define DBL_LIST_HEAD_INIT(name)        { &(name), &(name) }

#define DBL_LIST_HEAD(name)             struct dbl_list_head name = DBL_LIST_HEAD_INIT(name)

#define DBL_LIST_HEAD_STRUCT(name)	struct dbl_list_head name

#define DBL_LIST_TYPE_PTR	(struct dbl_list_head *)
#define DBL_LIST_TYPE	struct dbl_list_head
#define DBL_LIST_ENTRY_SIZE	sizeof(DBL_LIST_TYPE)

#define DBL_INIT_LIST_HEAD(ptr) \
do { \
    (ptr)->pNext = (ptr); (ptr)->pPrev = (ptr); \
} while (0)

#define DBL_LIST_ADD(newi, prev, next) \
do { \
    struct dbl_list_head *	pPrev = prev; \
    struct dbl_list_head *	pNext = next; \
    (pNext)->pPrev = newi; \
    (newi)->pNext = pNext; \
    (newi)->pPrev = pPrev; \
    (pPrev)->pNext = newi; \
} while (0)

#define DBL_LIST_ADDH(new, head)        DBL_LIST_ADD(new, head, (head)->pNext)

#define DBL_LIST_ADDT(new, head)        DBL_LIST_ADD(new, (head)->pPrev, head)

#define DBL_LIST_UNLINK(prev, next) \
do { \
    (next)->pPrev = prev; \
    (prev)->pNext = next; \
} while (0)

#define DBL_LIST_DEL(entry) \
do { \
    DBL_LIST_UNLINK((entry)->pPrev, (entry)->pNext); \
    DBL_INIT_LIST_HEAD(entry); \
} while (0)

#define DBL_LIST_EMTPY(head)            ((head)->pNext == head)

#define DBL_LIST_SPLICE(list, head) \
do { \
    struct dbl_list_head *    first = (list)->pNext; \
    if (first != list) { \
        struct dbl_list_head *	last = (list)->pPrev; \
        struct dbl_list_head *	at = (head)->pNext; \
        (first)->pPrev = head; \
        (head)->pNext = first; \
        (last)->pNext = at; \
        (at)->pPrev = last; \
    } \
} while (0)

#define DBL_HEAD_COPY(oldh, newh) \
do { \
    *(oldh) = (*newh); \
    (newh)->pNext->pPrev = (oldh); \
    (newh)->pPrev->pNext = (oldh); \
} while (0)

#define DBL_ITEM_IN_LIST(ptr)           (((ptr)->pPrev != (ptr)) && ((ptr)->pNext != (ptr)))

#define DBL_LIST_FIRST(head)            (((head)->pNext != (head)) ? (head)->pNext: NULL)

#define DBL_LIST_LAST(head)             (((head)->pPrev != (head)) ? (head)->pPrev: NULL)

#define DBL_LIST_ENTRY(ptr, type, member)   ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define DBL_LIST_FOR_EACH(pos, head)    for (pos = (head)->pNext; pos != (head); pos = (pos)->pNext)

#define DBL_END_OF_LIST(pos, head)      ((pos) == (head))

#define DBL_LIST_NEXT(pos, head)        (((pos)->pNext != (head)) ? (pos)->pNext: NULL)

#define DBL_LIST_PREV(pos, head)        (((pos)->pPrev != (head)) ? (pos)->pPrev: NULL)


/* add by albert, 2005-12-8 */
#define	DBL_LIST_DEL_FIRST(res, head)	\
do { \
    res = DBL_LIST_FIRST(head);\
    if (res) \
		DBL_LIST_DEL(res);\
} while (0)

#define	DBL_LIST_DEL_LAST(res, head)	\
do { \
    res = DBL_LIST_LAST(head);\
    if (res) \
		DBL_LIST_DEL(res);\
} while (0)

#define DBL_LIST_FOR_EACH_SAFE(pos, n, head)    for (pos = (head)->pNext, n = pos->pNext; pos != (head); pos = n, n = pos->pNext)

/* add done */

struct dbl_list_head
{
    struct dbl_list_head *pNext;
    struct dbl_list_head *pPrev;
};

#endif

