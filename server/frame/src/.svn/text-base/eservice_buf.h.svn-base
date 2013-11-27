#ifndef	_ESERVICE_BUF_H_albert_
#define	_ESERVICE_BUF_H_albert_	1

#include	"common_albert.h"
#include	"program_utils.h"
#include	"socket_utils.h"
#include	"dbllist.h"
#include	"albert_log.h"
#include	"event2/event.h"
#include	"event2/listener.h"
#include	"event2/bufferevent.h"
#include	"event2/buffer.h"

struct eservice_unit_t;
struct eservice_buf_t;
typedef int (*eservice_buf_process_callback)(struct eservice_unit_t *eup, struct eservice_buf_t *ebp, void *arg);

struct eservice_buf_t {
	struct dbl_list_head list;
	
	time_t use_time;
	struct evbuffer *evp;

	uint8_t retry_num;
	uint8_t pos_flag;
	uint8_t unused_1;
	uint8_t unused_2;
	eservice_buf_process_callback	send_failed_cb;
	void *arg;	
};

#define	ESERVICE_ESBUF2EVBUF(esp)	((esp)->evp)
#define MAX_G_BUFFER 1024 * 1024 * 2
#ifdef	__cplusplus
extern "C" {
#endif

typedef void (*eservice_buf_ref_cleanup_cb)(const void *data, size_t datalen, void *extra);

extern struct eservice_buf_t *eservice_buf_new(size_t init_size);
extern void eservice_buf_free(struct eservice_buf_t *ebp);
extern ssize_t eservice_buf_read(int fd, struct eservice_buf_t *ebp, ssize_t want_len);
extern ssize_t eservice_buf_datalen(struct eservice_buf_t *ebp);
extern uint8_t *eservice_buf_pos(struct eservice_buf_t *pkg, uint32_t from_head_len, ssize_t assure_linearizes_len);
extern uint8_t *eservice_buf_pos_ex(struct eservice_buf_t *pkg, uint32_t from_head_len, ssize_t assure_linearizes_len);
extern uint8_t *eservice_buf_start(struct eservice_buf_t *pkg);
extern uint8_t *eservice_buf_start_ex(struct eservice_buf_t *pkg);
extern int eservice_buf_prepend(struct eservice_buf_t *ebp, const void *data, size_t size);
extern int eservice_buf_make_linearizes(struct eservice_buf_t *ebp, ssize_t len);
extern time_t eservice_buf_get_usetime(struct eservice_buf_t *ebp);
extern void eservice_buf_set_usetime(struct eservice_buf_t *ebp, time_t cur_time);
extern int eservice_buf_set_control(struct eservice_buf_t *ebp, uint32_t retry_num, eservice_buf_process_callback failed_cb, void *arg);
extern int eservice_buf_get_space(struct eservice_buf_t *ebp, size_t want_len, uint8_t **space_posp, size_t *space_lenp);
extern int eservice_buf_commit_space(struct eservice_buf_t *ebp, uint8_t *space_pos, size_t space_len);
extern uint8_t *eservice_buf_find(struct eservice_buf_t *ebp, const unsigned char * what, size_t len);
extern int eservice_buf_drain(struct eservice_buf_t *ebp, size_t len);
extern int eservice_buf_add(struct eservice_buf_t *ebp, const void * data, size_t datlen);
extern int eservice_buf_add_reference(struct eservice_buf_t *ebp, const void *data, size_t datlen, eservice_buf_ref_cleanup_cb cleanupfn, void *cleanupfn_arg);
extern int eservice_buf_move_data(struct eservice_buf_t *dst, struct eservice_buf_t *src, ssize_t datalen);
extern struct eservice_buf_t *eservice_buf_list2buf(struct dbl_list_head *listp);
extern struct dbl_list_head *eservice_buf_buf2list(struct eservice_buf_t *ebp);

#ifdef	__cplusplus
}
#endif

#endif

