#ifndef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"ESERVICE_BUF"
#endif

#include	"eservice_buf.h"

extern uint32_t g_log_level;
extern CRollLog *g_albert_logp;			/* extern log */
uint8_t g_evbuffer_date[MAX_G_BUFFER];
uint32_t evbuff_num = 0;

struct eservice_buf_t *
eservice_buf_new(size_t init_size)
{
	struct eservice_buf_t *ebp = NULL;

	malloc_dbg(ebp, sizeof(eservice_buf_t), struct eservice_buf_t *);
	if (!ebp)
		return NULL;
	
	ebp->evp = evbuffer_new();
	if (!(ebp->evp)) {
		free_dbg(ebp);
		return NULL;
	}

	if (init_size > 0) {
		if (evbuffer_expand(ebp->evp, init_size) < 0) {
			evbuffer_free(ebp->evp);
			free_dbg(ebp);
			return NULL;
		}
	}

	ebp->pos_flag = 0;
	ebp->use_time = 0;
	DBL_INIT_LIST_HEAD(&(ebp->list));
	ebp->retry_num = 0;
	ebp->send_failed_cb = NULL;
	ebp->arg = NULL;
	evbuff_num ++;
	return ebp;
}

void
eservice_buf_free(struct eservice_buf_t *ebp)
{
	if (!ebp)
		return;
	
	evbuffer_free(ESERVICE_ESBUF2EVBUF(ebp));
	free_dbg(ebp);
	evbuff_num--;
	return;
}

time_t
eservice_buf_get_usetime(struct eservice_buf_t *ebp)
{
	if (ebp)
		return ebp->use_time;

	return 0;
}

void
eservice_buf_set_usetime(struct eservice_buf_t *ebp, time_t cur_time)
{
	if (ebp)
		ebp->use_time = cur_time;
}

ssize_t
eservice_buf_read(int fd, struct eservice_buf_t *ebp, ssize_t want_len)
{	
	if (want_len > 0) {
		evbuffer_expand(ESERVICE_ESBUF2EVBUF(ebp), want_len);
	}
	
	return evbuffer_read(ESERVICE_ESBUF2EVBUF(ebp), fd, want_len);
}

ssize_t
eservice_buf_datalen(struct eservice_buf_t *ebp)
{
	if (!ebp)
		return -1;
	
	return (ssize_t)(evbuffer_get_length(ESERVICE_ESBUF2EVBUF(ebp)));
}

int
eservice_buf_prepend(struct eservice_buf_t *ebp, const void *data, size_t size)
{
	if (!ebp)
		return -1;
	
	return evbuffer_prepend(ESERVICE_ESBUF2EVBUF(ebp), data, size);
}

uint8_t *
eservice_buf_pos_ex(struct eservice_buf_t *ebp, uint32_t from_head_len, ssize_t assure_linearizes_len)
{
	struct evbuffer *evp = NULL;
	int res = -255;
	struct evbuffer_ptr pos, *uptr = NULL;
	struct evbuffer_iovec *vec;
	uint8_t *ptr = NULL, *result_ptr = NULL;
	uint8_t vec_num = 0;

	if (!ebp)
		return NULL;

	evp = ESERVICE_ESBUF2EVBUF(ebp);
    size_t evbuf_len = evbuffer_get_length(evp);

    if (evbuf_len == 0 || evbuf_len > MAX_G_BUFFER)
    {
        g_warn("invalid evbuffer_get_length size=%zu", evbuf_len);
        return NULL;
    }
    if(assure_linearizes_len > MAX_G_BUFFER)
    {
        g_warn("invalid assure_linearizes_len=%zd", assure_linearizes_len);
        return NULL;
    }    

	if (from_head_len > 0) {
		res = evbuffer_ptr_set(evp, &pos, from_head_len, EVBUFFER_PTR_SET);
		if (res < 0) {
			g_warn("evbuffer_ptr_set() failed: %d", res);
			return NULL;
		}

		uptr = &pos;
	}
	else {
		uptr = NULL;
	}

	if (assure_linearizes_len == 0) {		/* need not linearizes */
		assure_linearizes_len = 1;
	}

	vec_num = evbuffer_peek(evp, assure_linearizes_len, NULL, NULL, 0);
	if(vec_num == 0)
    {
        g_warn("evbuffer_peek return 0");
		return NULL;
    }

	vec = (evbuffer_iovec *)malloc(sizeof(struct evbuffer_iovec) * vec_num);
	
	res = evbuffer_peek(evp, assure_linearizes_len, uptr, vec, vec_num);
	if (res <= 0) {
		g_warn("no this pos, from: %u", from_head_len);
		free(vec);
		return NULL;
	}

    // opt for common case
    if (vec[0].iov_len >= assure_linearizes_len)
    {
        g_dbg("vec[0].iov_len = %zu, assure_linearizes_len=%zu",
              vec[0].iov_len, assure_linearizes_len);
        result_ptr = (uint8_t *)(vec[0].iov_base);
        result_ptr += from_head_len;
        free(vec);
        return (result_ptr);
    }
	
	size_t pos_lenth = 0;
	for(size_t i = 0 ; i < vec_num ; ++i)
	{
		memcpy(g_evbuffer_date + pos_lenth, vec[i].iov_base, vec[i].iov_len);
		pos_lenth += vec[i].iov_len;
		if(pos_lenth > MAX_G_BUFFER){
			break;
		}
		g_dbg("vec_num, from: %d", vec_num);
	}

	result_ptr = &(g_evbuffer_date[from_head_len]);
	free(vec);
	return result_ptr;
}

uint8_t *
eservice_buf_pos(struct eservice_buf_t *ebp, uint32_t from_head_len, ssize_t assure_linearizes_len)
{
	struct evbuffer *evp = NULL;
	int res = -255;
	struct evbuffer_ptr pos, *uptr = NULL;
	struct evbuffer_iovec vec[2];
	uint8_t *ptr = NULL, *result_ptr = NULL;
	
	if (!ebp)
		return NULL;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);

	if (evbuffer_get_length(evp) == 0)
		return NULL;
	
	if (from_head_len > 0) {
		res = evbuffer_ptr_set(evp, &pos, from_head_len, EVBUFFER_PTR_SET);
		if (res < 0) {
			g_warn("evbuffer_ptr_set() failed: %d", res);
			return NULL;
		}

		uptr = &pos;
	}
	else {
		uptr = NULL;
	}

	if (assure_linearizes_len == 0) {		/* need not linearizes */
		assure_linearizes_len = 1;
	}
	
	res = evbuffer_peek(evp, assure_linearizes_len, uptr, vec, 2);
	if (res <= 0) {
		g_warn("no this pos, from: %u", from_head_len);
		return NULL;
	}
	
	if (res == 1 || assure_linearizes_len == 1) {
		result_ptr = (uint8_t *)(vec[0].iov_base);
	}
	else {							/* we should linearize it */
		g_warn("need linearizes");
		if (assure_linearizes_len < 0) {
			ptr = evbuffer_pullup(evp, -1);
			if (!ptr) {
				g_warn("make linearizes failed, len: -1");
				return NULL;
			}
		}
		else {
			ptr = evbuffer_pullup(evp, from_head_len + assure_linearizes_len);
			if (!ptr) {
				g_warn("make linearizes failed, len: %zu", from_head_len + assure_linearizes_len);
				return NULL;
			}
		}

		result_ptr = &(ptr[from_head_len]);
	}

	return result_ptr;
}

uint8_t *
eservice_buf_start_ex(struct eservice_buf_t *ebp)
{
	return eservice_buf_pos_ex(ebp, 0, 0);
}


uint8_t *
eservice_buf_start(struct eservice_buf_t *ebp)
{
	return eservice_buf_pos(ebp, 0, 0);
}

int
eservice_buf_make_linearizes(struct eservice_buf_t *ebp, ssize_t len)
{
	struct evbuffer *evp = NULL;
	
	if (!ebp)
		return -1;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);

	if (len < 0) {
		len = (ssize_t)(evbuffer_get_length(evp));
	}
	
	if (evbuffer_get_contiguous_space(evp) != (size_t)len) {
		g_warn("data not contiguous, ebp: %p, len: %zd", ebp, len);

		evbuffer_pullup(evp, len);
	}
	
	return 0;
}

int 
eservice_buf_set_control(struct eservice_buf_t *ebp, uint32_t retry_num, eservice_buf_process_callback failed_cb, void *arg)
{
	if (!ebp)
		return -1;

	ebp->retry_num = retry_num;
	ebp->send_failed_cb = failed_cb;
	ebp->arg = arg;
	
	return 0;
}

int
eservice_buf_get_space(struct eservice_buf_t *ebp, size_t want_len, uint8_t **space_posp, size_t *space_lenp)
{
	struct evbuffer *evp = NULL;
	int res = -255;
	struct evbuffer_iovec vec[1];
	
	if (!ebp)
		return -1;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);
	
	res = evbuffer_reserve_space(evp, (ev_ssize_t)want_len, vec, 1);			/* We need contiguous space. */
	if (res != 1) {
		g_warn("get contiguous space failed, want size: %zu, res: %d", want_len, res);
		return -2;
	}

	set_pval_type(space_posp, vec[0].iov_base, uint8_t *);
	set_pval_type(space_lenp, vec[0].iov_len, size_t);

	return 0;
}

int
eservice_buf_commit_space(struct eservice_buf_t *ebp, uint8_t *space_pos, size_t space_len)
{
	struct evbuffer *evp = NULL;
	int res = -255;
	struct evbuffer_iovec vec[1];
	
	if (!ebp || !space_pos)
		return -1;

	evp = ESERVICE_ESBUF2EVBUF(ebp);
	vec[0].iov_base = (void *)space_pos;
	vec[0].iov_len = space_len;
	
	res = evbuffer_commit_space(evp, vec, 1);
	if (res < 0) {
		g_warn("write data failed, pos: %p, len: %zu", space_pos, space_len);
		return res;
	}
	
	return 0;
}

uint8_t *
eservice_buf_find(struct eservice_buf_t *ebp, const uint8_t * what, size_t len)
{
	struct evbuffer *evp = NULL;
	struct evbuffer_ptr p;
	
	if (!ebp)
		return NULL;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);

	p = evbuffer_search(evp, (const char *)what, len, NULL);

	if (p.pos < 0)
		return NULL;

	return eservice_buf_pos(ebp, p.pos, -1);
}

int
eservice_buf_drain(struct eservice_buf_t *ebp, size_t len)
{
	struct evbuffer *evp = NULL;
	
	if (!ebp)
		return -1;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);

	return evbuffer_drain(evp, len);
}

int
eservice_buf_add(struct eservice_buf_t *ebp, const void * data, size_t datlen)
{
	struct evbuffer *evp = NULL;
	
	if (!ebp)
		return -1;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);

	return evbuffer_add(evp, data, datlen);
}

int
eservice_buf_add_reference(struct eservice_buf_t *ebp, const void *data, size_t datlen, eservice_buf_ref_cleanup_cb cleanupfn, void *cleanupfn_arg)
{
	struct evbuffer *evp = NULL;
	
	if (!ebp)
		return -1;
	
	evp = ESERVICE_ESBUF2EVBUF(ebp);

	return evbuffer_add_reference(evp, data, datlen, (evbuffer_ref_cleanup_cb)cleanupfn, cleanupfn_arg);
}

int eservice_buf_copy_data(struct eservice_buf_t *dst,struct eservice_buf_t *src)
{
	struct evbuffer *evp_src = NULL,*evp_dst;
	if(!src || !dst)
			return -1;
	evp_src = ESERVICE_ESBUF2EVBUF(src);
	evp_dst = ESERVICE_ESBUF2EVBUF(dst);
	return evbuffer_add_buffer_reference(evp_src,evp_dst);
}

int
eservice_buf_move_data(struct eservice_buf_t *dst, struct eservice_buf_t *src, ssize_t datalen)
{
	if (!src || !dst)
		return -1;

	if (datalen < 0) {
		return evbuffer_add_buffer(ESERVICE_ESBUF2EVBUF(dst), ESERVICE_ESBUF2EVBUF(src));
	}
	else {
		return evbuffer_remove_buffer(ESERVICE_ESBUF2EVBUF(src), ESERVICE_ESBUF2EVBUF(dst), datalen);
	}
}

struct eservice_buf_t *
eservice_buf_list2buf(struct dbl_list_head *listp)
{
	return DBL_LIST_ENTRY(listp, struct eservice_buf_t, list); 
}

struct dbl_list_head *
eservice_buf_buf2list(struct eservice_buf_t *ebp)
{
	return &(ebp->list);
}

