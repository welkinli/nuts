#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "ESERVICE_UTILS"
#endif

#include	"eservice_utils.h"
#include	"eservice_user.h"

/* global extern */
extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;
extern time_t g_current_time;

extern int g_unit_num;
extern uint64_t g_min_unit_address_val;
extern uint64_t g_max_unit_address_val;
extern uint64_t g_min_conn_context_address_val;
extern uint64_t g_max_conn_context_address_val;
int 
eservice_user_conn_idle_seconds(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	return (int)(g_current_time - eup->sp->last_active);
}

int 
eservice_user_conn_use_seconds(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	return (int)(g_current_time - eup->sp->use_time);
}

// uint32_t 
// eservice_user_get_cdnproxy_client_ip(eservice_unit_t *eup)
// {
// 	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
// 
// 	return (eup->sp->proxy_cdn_client_ip);
// }
// 
// uint32_t 
// eservice_user_set_cdnproxy_client_ip(eservice_unit_t *eup, uint32_t clientip)
// {
// 	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
// 
// 	eup->sp->proxy_cdn_client_ip = clientip;
// }

int 
eservice_user_conn_is_connecting(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	return (eup->sp->reconn_control.conn_in_progress > 0);
}

int
eservice_user_conn_set_user_data(eservice_unit_t *eup, void *user_data)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);

	eup->sp->user_data = user_data;
	
	return 0;
}

void *
eservice_user_conn_get_user_data(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, NULL);
	
	return eup->sp->user_data;
}

void
eservice_user_next_need(eservice_unit_t *eup, ssize_t want_len)
{
	if (!eup || !(eup->sp))
		return;

	eup->sp->pending_read_control.want_len = want_len;
	
	return;
}

int
eservice_user_pop_inbuf(eservice_unit_t *eup, ssize_t pop_size, struct eservice_buf_t **result_pkgp)
{
	ssize_t data_len = 0;
	struct eservice_buf_t *ebp = NULL;
	struct evbuffer *evp = NULL;
	int res = -255;

	if (!result_pkgp) {
		return eservice_user_errcode_invalid;
	}

	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	es_assign_frame_log();
	
	g_dbg("=> eservice_user_pop_inbuf(), eup: %p, sp: %p, pop_size: %zd", 
          eup, eup ? eup->sp : NULL, pop_size);
	
	data_len = eservice_buf_datalen(eup->sp->pending_read_control.buf);
	if (data_len < 0) {
		g_warn("eservice_buf_datalen() failed: %d, buf: %p", res, eup->sp->pending_read_control.buf);
		es_assign_plugin_log();
		return eservice_user_errcode_invalid;
	}
	
	if (pop_size > 0 && pop_size > data_len) {
		eservice_user_next_need(eup, pop_size - data_len);
		set_pval(result_pkgp, NULL);
		es_assign_plugin_log();
		return 0;
	}

	ebp = eservice_buf_new(0);
	if (!ebp) {
		g_warn("eservice_buf_new() failed: %s", strerror(errno));
		es_assign_plugin_log();
		return eservice_user_errcode_mm;
	}

	eservice_user_next_need(eup, -1);

	eservice_buf_set_usetime(ebp, g_current_time);

	if (pop_size < 0) {
		res = evbuffer_add_buffer(ESERVICE_ESBUF2EVBUF(ebp), ESERVICE_ESBUF2EVBUF(eup->sp->pending_read_control.buf));
	}
	else {
		res = evbuffer_remove_buffer(ESERVICE_ESBUF2EVBUF(eup->sp->pending_read_control.buf), ESERVICE_ESBUF2EVBUF(ebp), pop_size);
	}
	if (res < 0) 
    {
		g_warn("pop data failed: %s, eup: %p, pop size: %zd", 
               evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()), 
               eup, pop_size);

		es_assign_plugin_log();
		g_warn("pop data failed, eup: %p, pop size: %zd", eup, pop_size);
		eservice_buf_free(ebp);
		return res;
	}
	
	set_pval(result_pkgp, ebp);

	es_assign_plugin_log();
	return 0;
}

int 
eservice_user_push_outbuf(eservice_unit_t *eup, struct eservice_buf_t *ebp)
{
	int res = -255;

	if (!ebp) {
		return eservice_user_errcode_invalid;
	}

	if (ES_INVALID_EUP(eup)) {
		g_warn("the connection is not valid any longer: %p", eup);
		eservice_buf_free(ebp);
		return eservice_user_errcode_invalid;
	}
	
	es_assign_frame_log();
	
	g_dbg("=> eservice_user_push_outbuf(), eup: %p, sp: %p, ebp: %p", eup, eup ? eup->sp : NULL, ebp);
	
	DBL_LIST_DEL(&(ebp->list));

	if (eup->sp->waiting_write_control.n_to_write >= ES_TCP_MAX_QUEUE_WRITE) {
		es_assign_plugin_log();
		g_warn("too many pending write: %u", eup->sp->waiting_write_control.n_to_write);
		eservice_buf_free(ebp);
		return eservice_user_errcode_write;
	}

	eservice_buf_set_usetime(ebp, g_current_time);
	
	DBL_LIST_ADDT(&(ebp->list), &(eup->sp->waiting_write_control.sending_list_header));
	eup->sp->waiting_write_control.n_to_write++;
	
	res = es_conn_context_write(eup->sp);
	if (res < 0) {
		es_assign_plugin_log();
		g_warn("write connection failed: %d", res);
		return eservice_user_errcode_write;
	}

	es_assign_plugin_log();
	return 0;
}

int
eservice_user_set_close(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	eup->sp->pending_read_control.close_after_next_write = 1;
	
	return 0;
}

uint8_t *
eservice_user_find(eservice_unit_t *eup, const uint8_t * what, size_t len)
{
	ES_INVALID_EUP_RET(eup, NULL);
	
	return eservice_buf_find(eup->sp->pending_read_control.buf, what, len);
}

int
eservice_user_drain(eservice_unit_t *eup, size_t len)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	return eservice_buf_drain(eup->sp->pending_read_control.buf, len);
}

ssize_t
eservice_user_datalen(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	return eservice_buf_datalen(eup->sp->pending_read_control.buf);
}

uint8_t *
eservice_user_pos_ex(eservice_unit_t *eup, uint32_t from_head_len, ssize_t assure_linearizes_len)
{
	ES_INVALID_EUP_RET(eup, NULL);

	return eservice_buf_pos_ex(eup->sp->pending_read_control.buf, from_head_len, assure_linearizes_len);
}

uint8_t *
eservice_user_pos(eservice_unit_t *eup, uint32_t from_head_len, ssize_t assure_linearizes_len)
{
	ES_INVALID_EUP_RET(eup, NULL);
	
	return eservice_buf_pos(eup->sp->pending_read_control.buf, from_head_len, assure_linearizes_len);
}

uint8_t *
eservice_user_start(eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, NULL);
	
	return eservice_buf_start(eup->sp->pending_read_control.buf);
}

struct eservice_unit_t *
eservice_user_register_tcp_conn(struct eservice_unit_t *already_eup, const char *ip, const char *port, int reconn_ms, struct timeval *to)
{
	struct sockaddr_in srv;
	es_conn_context *sp = NULL;
	
	if (!ip || !port) {
		g_warn("invalid argument, ip: %p, port: %p", ip, port);
		return NULL;
	}

	//es_assign_frame_log();
	
	if (already_eup) {
		es_destroy_tcp_conn(already_eup->sp);
	}
	
	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = inet_addr(ip);
	srv.sin_port = htons(atoi(port));
	
	sp = es_create_tcp_conn(&srv, reconn_ms, to);
	if (!sp) {
		es_assign_plugin_log();
		return NULL;
	}

	g_dbg("=> eservice_user_register_tcp_conn(), result eup: %p, result sp: %p, ip: %s, port: %s, reconn ms: %d", sp ? sp->linked_eup : NULL, sp, ip, port, reconn_ms);
	
	//es_assign_plugin_log();
	return sp->linked_eup;
}

struct eservice_unit_t *
eservice_user_register_tcp_imme(struct eservice_unit_t *already_eup, const char *ip, const char *port, int reconn_ms, struct timeval *to)
{
	struct sockaddr_in srv;
	es_conn_context *sp = NULL;
	
	if (!ip || !port) {
		g_warn("invalid argument, ip: %p, port: %p", ip, port);
		return NULL;
	}

	es_assign_frame_log();
	
	if (already_eup) {
		es_destroy_tcp_conn(already_eup->sp);
	}
	
	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = inet_addr(ip);
	srv.sin_port = htons(atoi(port));
	
	sp = es_create_tcp_imme(&srv, reconn_ms, to);
	if (!sp) {
		es_assign_plugin_log();
		return NULL;
	}
	
	es_assign_plugin_log();

	g_dbg("=> eservice_user_register_tcp_imme(), result eup: %p, result sp: %p, ip: %s, port: %s, reconn ms: %d", sp ? sp->linked_eup : NULL, sp, ip, port, reconn_ms);
	
	return sp->linked_eup;
}

struct sockaddr_in *
eservice_user_get_register_tcp_peer(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, NULL);

	return &(eup->sp->peer);
}

int 
eservice_user_set_tcp_conn_timeout_cb(struct eservice_unit_t *eup, eservice_user_process_callback conn_timeout_cb, void *conn_timeout_arg, uint32_t to_ms)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);

	msec2timeval(&(eup->sp->to), (long)to_ms);
	eup->sp->per_conn_cbs.tcp_conn_timeout.user_callback_func = conn_timeout_cb;
	eup->sp->per_conn_cbs.tcp_conn_timeout.earg = conn_timeout_arg;
	
	return 0;
}

int
eservice_user_set_tcp_rw_buffer_size(struct eservice_unit_t *eup, uint32_t read_buffer_size, uint32_t write_buffer_size)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);

	set_recvbuf_size(eup->sp->fd, read_buffer_size);
	set_sendbuf_size(eup->sp->fd, write_buffer_size);
	
	return 0;
}

int 
eservice_user_set_tcp_conn_data_arrive_cb(struct eservice_unit_t *eup, eservice_user_process_callback data_arrive_cb, void *data_arrive_arg)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);

	eup->sp->per_conn_cbs.data_arrive.user_callback_func = data_arrive_cb;
	eup->sp->per_conn_cbs.data_arrive.earg = data_arrive_arg;
		
	return 0;
}

int 
eservice_user_set_tcp_conn_terminate_cb(struct eservice_unit_t *eup, eservice_user_process_callback terminate_cb, void *terminate_arg)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);

	eup->sp->per_conn_cbs.tcp_terminate.user_callback_func = terminate_cb;
	eup->sp->per_conn_cbs.tcp_terminate.earg = terminate_arg;
		
	return 0;
}

int
eservice_user_destroy_tcp_conn(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, eservice_user_errcode_invalid);
	
	return es_destroy_tcp_conn(eup->sp);
}

const char *
eservice_user_get_peer_addr_str(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, NULL);
	return inet_ntoa(eup->sp->peer.sin_addr);
}

network32_t
eservice_user_get_peer_addr_network(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, 0);
	return (eup->sp->peer.sin_addr.s_addr);
}

host16_t
eservice_user_get_peer_addr_port(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, 0);
	return (uint16_t)(ntohs(eup->sp->peer.sin_port));
}

const char *
eservice_user_get_local_addr_str(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, NULL);
	return inet_ntoa(eup->sp->related_native.sin_addr);
}

network32_t
eservice_user_get_local_addr_network(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, 0);
	return (eup->sp->related_native.sin_addr.s_addr);
}

host16_t 
eservice_user_get_local_addr_port(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, 0);
	return (uint16_t)(ntohs(eup->sp->related_native.sin_port));
}
         
host16_t 
eservice_user_get_related_port(struct eservice_unit_t *eup)
{
	ES_INVALID_EUP_RET(eup, 0);
	return (uint16_t)(eup->sp->related_port);
}

int
eservice_user_timer(struct eservice_manager_t *egr, struct eservice_timer_t *timerp, struct timeval *to, void (*eservice_user_timer_cb)(int, short, void *), void *arg)
{
	if (!egr || !timerp || !to || !eservice_user_timer_cb)
		return -1;

	if (!(timerp->timer_event)) {
		timerp->timer_event = evtimer_new(egr->ebase, eservice_user_timer_cb, arg);
	}
	
	if (evtimer_add(timerp->timer_event, to) < 0) {
		g_warn("timer failed: %s\n", strerror(errno));
		return -2;
	}
	
	return 0;
}

int
eservice_user_eup_is_invalid(struct eservice_unit_t *eup)
{
	if ((uint64_t)eup < g_min_unit_address_val || (uint64_t)eup > g_max_unit_address_val)
		return 1;

	if (eup->used == 0 || !(eup->sp))
		return 1;

	if ((uint64_t)(eup->sp) < g_min_conn_context_address_val || (uint64_t)(eup->sp) > g_max_conn_context_address_val)
		return 1;
	
	if (eup->seq != eup->sp->seq || ES_INVALID_CONN(eup->sp) || (uint64_t)(eup->sp->linked_eup) != (uint64_t)eup)
		return 1;

    if (eup->sp->type == es_socket_type_tcp_listen)
        return 1;
	
	return 0;
}

int
eservice_user_set_tcp_nodelay(struct eservice_unit_t *eup, int is_nodelay)
{
	ES_INVALID_EUP_RET(eup, -1);

	return set_tcp_nodelay(eup->sp->fd, is_nodelay);
}


int
eservice_user_eup_to_index(struct eservice_unit_t *eup)
{
    //if (eservice_user_eup_is_invalid(eup)) return -1;
    ES_INVALID_EUP_RET(eup, -1); // 

    eservice_unit_t *start = (eservice_unit_t *)g_min_unit_address_val;
    return (eup - start);
}

eservice_unit_t *
eservice_user_eup_from_index(int index)
{
    if (index < 0 || index > g_unit_num) return NULL;

    eservice_unit_t *start = (eservice_unit_t *)g_min_unit_address_val;
    return start + index;
}

