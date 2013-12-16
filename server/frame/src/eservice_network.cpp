#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "ESERVICE_NETWORK"
#endif

#include	"eservice_network.h"
#include	"eservice_utils.h"

#include <sys/mman.h>

#include <stdlib.h>
/* global extern */
extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;
extern time_t g_current_time;
extern struct timeval g_current_tv;
extern uint32_t now_terminate;
extern struct timeval g_tcp_conn_timeout;
extern struct timeval g_event_timeout;
extern eservice_manager_t g_es_manager;

extern CRollLog *g_es_frame_logp;
extern uint32_t g_es_frame_log_level;

extern CRollLog *g_es_user_plugin_logp;
extern uint32_t g_es_user_plugin_log_level;
extern eservice_main_conf_t g_es_main_conf;
extern uint32_t g_acceptfd_lowlimit;
extern uint32_t g_max_connection_num;
/* static define */
static es_conn_context *g_socket_context_chunk = NULL;
static struct LRU_node  g_tail_lru_node;
static uint32_t g_lru_num = 0;
static es_avaiable_head g_avaiable;
static struct eservice_unit_t *g_unit_chunk = NULL;
static es_avaiable_head g_unit_avaiable;
static struct event *sigpipe_event;		/* for SIGPIPE */
static struct event *sigusr1_event;     /* for SIGUSR1 */
static struct event *one_sec_timer_event;		/* for 1s timer */
static struct event *s30_sec_timer_event;		/* for 30s timer */
static struct timeval *g_all_connfd_timeout_p = NULL;
static uint32_t g_socket_context_seq = 1;
static uint32_t g_removefd =0;

/* global define */
int g_unit_num = 0;
uint64_t g_min_unit_address_val = 0;
uint64_t g_max_unit_address_val = 0;
uint64_t g_min_conn_context_address_val = 0;
uint64_t g_max_conn_context_address_val = 0;
uint64_t g_total_connection_num = 0;
/* listen fds */
uint32_t listen_fd_num = 0;
int listen_fd_array[ES_TCP_MAX_LISTEN_PORT];
int listen_fd_proto_array[ES_TCP_MAX_LISTEN_PORT];
es_conn_context *listen_esp_array[ES_TCP_MAX_LISTEN_PORT];
port_statistic_t port_statistic_array[ES_TCP_MAX_LISTEN_PORT];
network32_t process_spe_listen_ip = 0;
uint16_t process_spe_listen_port = 0;

/* static function declare */
static void es_listenfd_read_cb(int fd, short flag, void *arg);
static void es_fd_event_cb(int fd, short flag, void *arg);
static void es_udpfd_event_cb(int fd, short flag, void *arg);

#define accept_fd_num (g_es_main_conf.max_conn_num - g_avaiable.total_num)

uint32_t *shm_ptr = NULL;
uint32_t shm_index = -1;

#if 0

static void
eservice_echo_read_cb(struct bufferevent *bev, void *ctx)
{
	char tmp_buf1[32 * 1024];
	char tmp_buf2[32 * 1024];
	char tmp_buf3[32 * 1024];

        /* This callback is invoked when there is data to read on bev. */
        struct evbuffer *input = bufferevent_get_input(bev);
        struct evbuffer *output = bufferevent_get_output(bev);

	memcpy(tmp_buf1, tmp_buf2, sizeof(tmp_buf2));
	memcpy(tmp_buf1, tmp_buf3, sizeof(tmp_buf3));
	
        /* Copy all the data from the input buffer to the output buffer. */
        evbuffer_add_buffer(output, input);

	return;
}

static void
eservice_echo_write_cb(struct bufferevent *bev, void *ctx)
{
	g_dbg("eservice_echo_write_cb() called, bev: %p, ctx: %p", bev, ctx);

	return;
}

static void
eservice_echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
        if (events & BEV_EVENT_ERROR) {
                g_warn("Error from bufferevent: %s", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        }
		
        if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		g_dbg("close connection and free buffer");
                bufferevent_free(bev);
        }

	return;
}

static void
eservice_accept_echo_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{
        /* We got a new connection! Set up a bufferevent for it. */
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(bev, eservice_echo_read_cb, eservice_echo_write_cb, eservice_echo_event_cb, NULL);

        bufferevent_enable(bev, EV_READ|EV_WRITE);
}

static void
eservice_accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
	g_warn("Got an error %d (%s) on the listener.\n""Shutting down.\n", err, evutil_socket_error_to_string(err));

        event_base_loopexit(base, NULL);
}
#endif

static inline void
__reset_tcp_conn_timeout(struct timeval *tvp)
{
	tvp->tv_sec = g_tcp_conn_timeout.tv_sec;
	tvp->tv_usec = g_tcp_conn_timeout.tv_usec;
	return;
}

static inline void
__reset_event_timeout(struct timeval *tvp)
{
	tvp->tv_sec = g_event_timeout.tv_sec;
	tvp->tv_usec = g_event_timeout.tv_usec;
	return;
}

static void
__sigpipe_callback(int signal, short flag, void *arg)
{
	/* just ignore */
	g_warn("caught SIGPIPE, ignore !!!\n");
	return;
}

static void
__sigusr1_callback(int signal, short flag, void *arg)
{
    g_warn("caught SIGUSR1, reload config\n");
    eservice_manager_t *mgr = (eservice_manager_t *)arg;

    int res = 0;
	/* call user mod init */
	CALL_USER_PLUGIN(res, mgr->cbs.mod_init.user_callback_func, mgr, NULL, NULL, mgr->garg);
	if (res < 0) {
		g_warn("user mod reload failed: %d", res);
	}

    return ;
}

static void del_listen_fd()
{
    es_conn_context *con_p = NULL;
    uint32_t i;
    //g_dbg("accept_fd_num:%d,config:%d, lowlimited :%d",accept_fd_num, g_es_main_conf.acceptfd_limit, g_acceptfd_lowlimit);

    if (accept_fd_num >= g_es_main_conf.acceptfd_limit && (g_removefd ==0))//本进程过载
    {

        for(i =0; i<g_es_main_conf.process_num; i++ )
        {
            if(shm_ptr[i] < g_es_main_conf.acceptfd_limit)
            {
                break;
            }
        }

        if (i < g_es_main_conf.process_num)//其他进程至少有一个没有过载，需要删除listen fd
        {
            for (i = 0; i < ES_TCP_MAX_LISTEN_PORT; i++)
            {
                if (listen_fd_proto_array[i] == SOCK_STREAM)
                {
                    con_p = listen_esp_array[i];
                    event_del(con_p->ev);
                }

                if (listen_fd_proto_array[i] == 0)
                    break;
            }
            g_removefd = 1;
            g_dbg("del listen fd\n");
        }
    }
    return ;
}

static void add_listen_fd()
{
    es_conn_context *con_p = NULL;
    uint32_t i;

    if (g_removefd && (accept_fd_num < (g_acceptfd_lowlimit)))
    {
        for (i = 0; i < ES_TCP_MAX_LISTEN_PORT; i++)
        {
            if (listen_fd_proto_array[i] == SOCK_STREAM)
            {
                con_p = listen_esp_array[i];
                if (g_es_main_conf.listen_timeout > 0)
                {
                    __reset_event_timeout(&(con_p->to));
                    event_add(con_p->ev, &(con_p->to));
                }
                else
                {
                    event_add(con_p->ev, NULL);
                }
            }

            if (listen_fd_proto_array[i] == 0)
                break;
        }
        g_removefd = 0;
        g_dbg("add listen fd\n");
    }
}

static bool is_overload()
{
    uint32_t i =0;
    for (i = 0; i < g_es_main_conf.process_num; i++)
    {
        if (shm_ptr[i] < g_es_main_conf.acceptfd_limit)
        {
            break;
        }
    }

    if (i == g_es_main_conf.process_num)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void deal_overload()
{

    if(is_overload() && g_removefd) //all process over load
    {
        es_conn_context *con_p = NULL;
        uint32_t j;
        for (j= 0; j < ES_TCP_MAX_LISTEN_PORT; j++)
        {
            if (listen_fd_proto_array[j] == SOCK_STREAM)
            {
                con_p = listen_esp_array[j];
                if (g_es_main_conf.listen_timeout > 0)
                {
                    __reset_event_timeout(&(con_p->to));
                    event_add(con_p->ev, &(con_p->to));
                }
                else
                {
                    event_add(con_p->ev, NULL);
                }
            }

            if (listen_fd_proto_array[j] == 0)
                break;
        }
        g_removefd = 0;

        g_dbg("deal_overload all process are overload,add listen fd\n");

    }
}

static void get_total_fdnum()
{
    uint32_t i;
    uint64_t total_connection_num = 0;
    for(i =0; i<g_es_main_conf.process_num; i++)
    {
        total_connection_num += shm_ptr[i];
    }

    g_total_connection_num = total_connection_num;
}

static void clear_lru()
{
	//log LRU list
// 	dbl_list_head * tmpnode = &g_tail_lru_node.list; 
// 	while(tmpnode->pNext != &g_tail_lru_node.list)
// 	{
// 		LRU_node *lru_node = DBL_LIST_ENTRY(tmpnode->pNext, LRU_node, list);
// 		g_dbg("this node :0x%x es_conn_context: 0x%x" , (int)tmpnode->pNext, (int )lru_node->sp);
// 		tmpnode = tmpnode->pNext;
// 	}
	//clear LRU list
    uint32_t used_lru_num = accept_fd_num;
    if(accept_fd_num >= g_acceptfd_lowlimit - 10)
    {
        int count = accept_fd_num / 100 * 20;
        int clear_num = 0;

        dbl_list_head * tmpnode = &g_tail_lru_node.list; 
        for (clear_num = 0; clear_num < count && tmpnode->pNext != &g_tail_lru_node.list; ++clear_num)
        //while(count > 0 && tmpnode->pNext != &g_tail_lru_node.list)
        {
            LRU_node *lru_node = DBL_LIST_ENTRY(tmpnode->pNext, LRU_node, list);
            g_msg("NO.%d, clear node:%p es_conn_context: %p" , 
                  clear_num, tmpnode->pNext, lru_node->sp);

            es_destroy_tcp_conn((es_conn_context *)(lru_node->sp));
            tmpnode = tmpnode->pNext;
        }
        g_warn(" clear node used num:%d  finished lru_num:%d clearnode_num:%d", used_lru_num, accept_fd_num, clear_num);
    }
}

static void
__timer1sec_callback(int fd, short flag, void *arg)
{

	uint32_t i;
	struct timeval one_sec;

	gettimeofday(&g_current_tv, NULL);
	g_current_time = g_current_tv.tv_sec;

	g_socket_context_seq = (uint32_t)(random());
	if (g_socket_context_seq == 0)
		g_socket_context_seq = 1;

#if 0
	for (i = 0; i < listen_fd_num; i++) {
		if (port_statistic_array[i].used) {
			g_warn("= port statistic: %u, req: %u", port_statistic_array[i].port, port_statistic_array[i].recv_num);
			port_statistic_array[i].recv_num = 0;
		}
		else {
			break;
		}
	}
	
	g_warn("== free conn context: %d", g_avaiable.total_num);
#endif

	one_sec.tv_sec = 1;
	one_sec.tv_usec = 0;
	if (evtimer_add(one_sec_timer_event, &one_sec) < 0) {
		//g_warn("loop 1s timer failed");
	}

	 for (i = 0; i < g_es_main_conf.process_num; i++)
    {
	     //g_warn("shm_ptr[%d]:%d",i,shm_ptr[i]);
    }

	//g_warn("Had Accept_FD_num:%d , limited config:%d, lowlimited config:%d", accept_fd_num , g_es_main_conf.acceptfd_limit, g_acceptfd_lowlimit);
	return;
}

static void
__timer30sec_callback(int fd, short flag, void *arg)
{
	uint32_t i;
	struct timeval s30_sec;

	gettimeofday(&g_current_tv, NULL);
	g_current_time = g_current_tv.tv_sec;

#if 0
	for (i = 0; i < listen_fd_num; i++) {
		if (port_statistic_array[i].used) {
			g_warn("= port statistic: %u, req: %u", port_statistic_array[i].port, port_statistic_array[i].recv_num);
			port_statistic_array[i].recv_num = 0;
		}
		else {
			break;
		}
	}
#endif

	g_warn("== free conn context: %d", g_avaiable.total_num);
	
	s30_sec.tv_sec = 30;
	s30_sec.tv_usec = 0;
	if (evtimer_add(s30_sec_timer_event, &s30_sec) < 0) {
		g_warn("loop 30s timer failed");
	}
	
	return;
}

static void
__write_to_mylog_cb(int severity, const char *msg)
{
	//GDETAIL(G_ALOG_LEVEL_DBG - _EVENT_LOG_DEBUG, msg);
	switch (severity) {
		case _EVENT_LOG_DEBUG: 
			g_dbg("%s", msg);
			break;
			
		case _EVENT_LOG_MSG:  
			g_msg("%s", msg);
			break;
			
		case _EVENT_LOG_WARN:
		case _EVENT_LOG_ERR:
			g_warn("%s", msg);
			
		default:
			break; /* never reached */
	}

	return;
}

static struct event_base *
__get_strongest_event_backend(void)
{
	struct event_base *ebase;
	enum event_method_feature f;
	struct event_config *econfp = NULL;
	
	econfp = event_config_new();
	if (!econfp) {
		g_err("event_config_new() failed: %s", strerror(errno));
	}

	if (event_config_require_features(econfp, EV_FEATURE_O1) < 0) {
		g_err("event_config_require_features() failed: %s", strerror(errno));
	}
	
	if (event_config_set_flag(econfp, EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST) < 0) {
		g_err("event_config_set_flag() failed: %s", strerror(errno));
	}
	
	ebase = event_base_new_with_config(econfp);
	if (!ebase) {
    		g_err("Couldn't get an event_base!");
	}

	g_msg("Using Libevent with backend method %s.", event_base_get_method(ebase));

	f = (enum event_method_feature)event_base_get_features(ebase);
	if ((f & EV_FEATURE_ET)) {
		g_msg("  Edge-triggered events are supported.");
	}
	if ((f & EV_FEATURE_O1)) {
		g_msg("  O(1) event notification is supported.");
	}
	if ((f & EV_FEATURE_FDS)) {
		g_msg("  All FD types are supported.");
	}

	if (event_base_priority_init(ebase, ESERVICE_PRIORITY_NUM) < 0) {
		g_warn("event_base_priority_init() failed");
	}
	
	return ebase;
}

static int
__set_resource(eservice_main_conf_t *confp)
{
	struct rlimit rl;
	uint32_t file_limit = 0;

	memset(&rl, 0, sizeof(rl));
	
	if (confp->max_conn_num + 100 >= 1024) {
		getrlimit(RLIMIT_NOFILE, &rl);
		file_limit = confp->max_conn_num;
		file_limit = MIN(file_limit, ESERVICE_SYSTEM_MAX_FILENO);
		rl.rlim_cur = rl.rlim_max = file_limit;
		if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
			g_warn("setrlimit() failed: %s", strerror(errno));
			return -1;
		}
	}

	return 0;
}

static int
__init_global_context(eservice_manager_t *mgr, eservice_main_conf_t *confp)
{
	int i;
	es_conn_context *ptr = NULL;
	struct eservice_unit_t *uptr = NULL;
	uint32_t num = 0;
	
	/* socket context */
	g_socket_context_chunk = (es_conn_context *)malloc(confp->max_conn_num * sizeof(es_conn_context));
	if (!g_socket_context_chunk) {
		g_warn("g_mem_chunk_new() for socket context failed: %s", strerror(errno));
		return -1;
	}
	
	g_avaiable.total_num = 0;
	g_lru_num = 0;
	DBL_INIT_LIST_HEAD(&(g_avaiable.head));
	DBL_INIT_LIST_HEAD(&(g_tail_lru_node.list));

	for (i = 0; i < (int)confp->max_conn_num; i++) {
		ptr = (es_conn_context *)(&(g_socket_context_chunk[i]));

		/*init LRU node*/
		DBL_INIT_LIST_HEAD(&(g_socket_context_chunk[i].lru_node.list));
		g_socket_context_chunk[i].lru_node.sp = (uint64_t *)ptr;

		if (i == 0) {
			g_min_conn_context_address_val = (uint64_t)ptr;
			g_dbg("min sp val: %p", ptr);
		}
		
		if (i == (int)(confp->max_conn_num) - 1) {
			g_max_conn_context_address_val = (uint64_t)ptr;
			g_dbg("max sp val: %p", ptr);
		}
		
		/* init socket context */
		ptr->status = es_context_status_unused;
	
		/* push */
		DBL_INIT_LIST_HEAD(&(ptr->list));
		DBL_LIST_ADDT(&(ptr->list), &(g_avaiable.head));
		g_avaiable.total_num++;
	}

	/* unit twice than context for link rand */
	num = 2 * confp->max_conn_num;
	g_unit_chunk = (struct eservice_unit_t *)malloc(num * sizeof(struct eservice_unit_t));
	if (!g_unit_chunk) {
		g_warn("g_mem_chunk_new() for unit failed: %s", strerror(errno));
		return -1;
	}
	
	g_unit_num = num;
	g_unit_avaiable.total_num = 0;
	DBL_INIT_LIST_HEAD(&(g_unit_avaiable.head));

	for (i = 0; i < (int)num; i++) {
		uptr = (struct eservice_unit_t *)(&(g_unit_chunk[i]));

		if (i == 0) {
			g_min_unit_address_val = (uint64_t)uptr;
			g_dbg("min unit val: %p", uptr);
		}
		
		if (i == (int)(num) - 1) {
			g_max_unit_address_val = (uint64_t)uptr;
			g_dbg("max unit val: %p", uptr);
		}
		
		/* init socket context */
		uptr->used = 0;
	
		/* push */
		DBL_INIT_LIST_HEAD(&(uptr->list));
		DBL_LIST_ADDT(&(uptr->list), &(g_unit_avaiable.head));
		g_unit_avaiable.total_num++;
	}
	
	return 0;
}

static inline int
__free_used_slot(es_conn_context *sc)
{
	if (sc->linked_eup) {
		sc->linked_eup->used = 0;
		sc->linked_eup->seq = 0;
		sc->linked_eup->sp = NULL;
		DBL_LIST_DEL(&(sc->linked_eup->list));
		DBL_LIST_ADDT(&(sc->linked_eup->list), &(g_unit_avaiable.head));
		g_unit_avaiable.total_num++;
	}

	sc->status = es_context_status_unused;
	sc->seq = 0;
	sc->linked_eup = NULL;
	sc->fd = -1;
	sc->pending_read_control.close_after_next_write = 0;
	
	DBL_LIST_DEL(&(sc->lru_node.list));
	g_lru_num--;

	DBL_LIST_DEL(&(sc->list));
	DBL_LIST_ADDT(&(sc->list), &(g_avaiable.head));
	g_avaiable.total_num++;

	return 0;
}

static inline es_conn_context *
__find_empty_slot(void)
{
	es_conn_context *ptr = 0;
	struct eservice_unit_t *eup = NULL;
	struct dbl_list_head *lh = NULL;

	/* socket context */
	if (g_avaiable.total_num == 0)
		ptr = NULL;
	else {
		lh = DBL_LIST_FIRST(&(g_avaiable.head));
		DBL_LIST_DEL(lh);
		g_avaiable.total_num--;
		ptr = DBL_LIST_ENTRY(lh, es_conn_context, list);
	}

	if (!ptr) {
		g_warn("g_avaiable queue is empty");
		return NULL;
	}
	
	ptr->ev = NULL;
	ptr->status = es_context_status_unused;
	ptr->seq = g_socket_context_seq;
	
	g_socket_context_seq++;
	if (g_socket_context_seq == 0)
		g_socket_context_seq = 1;

	/* link unit */
	if (g_unit_avaiable.total_num == 0) {
		g_warn("g_avaiable unit queue is empty");
		__free_used_slot(ptr);
		return NULL;
	}
	
	lh = DBL_LIST_FIRST(&(g_unit_avaiable.head));
	DBL_LIST_DEL(lh);
	g_unit_avaiable.total_num--;
	eup = DBL_LIST_ENTRY(lh, struct eservice_unit_t, list);
	eup->used = 1;	
	eup->seq = ptr->seq;

	/* point each other */
	eup->sp = ptr;
	ptr->linked_eup = eup;
	
	return ptr;
}

static inline void
__clean_read_control(es_conn_context *sc)
{
	if (sc->pending_read_control.buf) {
		eservice_buf_free(sc->pending_read_control.buf);
		sc->pending_read_control.buf = NULL;
	}
	sc->pending_read_control.want_len = -1;

	return;
}

static void
__clean_write_control(es_conn_context *sc)
{
	struct dbl_list_head *ptr = NULL, *n = NULL;
	struct eservice_buf_t *ebp = NULL;
	int res = -255;
	
	DBL_LIST_FOR_EACH_SAFE(ptr, n, &(sc->waiting_write_control.sending_list_header)) {
		DBL_LIST_DEL(ptr);
		ebp = DBL_LIST_ENTRY(ptr, struct eservice_buf_t, list);

		/* notice user */
		if (ebp->send_failed_cb) {
			CALL_USER_BUF_PLUGIN(res, ebp->send_failed_cb, sc->linked_eup, ebp, ebp->arg);
		}
		
		eservice_buf_free(ebp);
		ebp = NULL;
	}
	
	DBL_INIT_LIST_HEAD(&(sc->waiting_write_control.sending_list_header));
	sc->waiting_write_control.n_to_write = 0;

	return;
}

static int
_close_tcp_context(es_conn_context *sc)
{
	if (!sc) {
		return eservice_errcode_param_invalid;
	}

	/* first of all */
	if (sc->ev) {
		event_del(sc->ev);
	}
	
	/* shut down network */
	if (sc->fd >= 0)
		shutdown(sc->fd, SHUT_RDWR);
	
	close_socket(sc->fd);

	/* clear event */
	if (sc->ev) {
		event_free(sc->ev);
	}
	sc->ev = NULL;
	sc->last_event = 0;
	
	/* clear reconn control */
	if (sc->reconn_control.reconn_timerev) {
		event_free(sc->reconn_control.reconn_timerev);
	}
	sc->reconn_control.reconn_timerev = NULL;
	sc->reconn_control.conn_in_progress = 0;
	sc->reconn_control.last_written_data_len = -1;
	
	return 0;
}

static int
_free_tcp_context(es_conn_context *sc)
{
	_close_tcp_context(sc);

	__clean_read_control(sc);
	__clean_write_control(sc);
	
	return __free_used_slot(sc);
}

static int
_close_read_tcp_context(es_conn_context *sc)
{
	if (!sc) {
		return eservice_errcode_param_invalid;
	}

	event_free(sc->ev);

	if (sc->fd >= 0)
		shutdown(sc->fd, SHUT_RD);

	__clean_read_control(sc);
	
	return 0;
}

static int
_clean_udp_context(es_conn_context *sc)
{
	if (!sc) {
		return eservice_errcode_param_invalid;
	}

	__clean_read_control(sc);
	
	return 0;
}

static inline int
__make_slot(es_conn_context *scp, int port, int fd, int type, struct sockaddr_in *addrp, eservice_manager_t *mgr, struct sockaddr_in *peerp, struct event_base *ebase, int reconn_ms, struct timeval *to)
{
	if (!scp)
		return eservice_errcode_param_invalid;

	if (!addrp) {			/* client connection not connected yet */
		memset(&(scp->related_native), 0, sizeof(struct sockaddr_in));
	}
	else {
		memcpy(&(scp->related_native), addrp, sizeof(struct sockaddr_in));
	}

	scp->status = es_context_status_used;
	scp->type = type;
	scp->fd = fd;	
	scp->related_port = port;
	scp->egr = mgr;
	scp->ebase = ebase;
	scp->last_event = 0;

	if (peerp) {
		memcpy(&(scp->peer), peerp, sizeof(scp->peer));
	}
	else {
		memset(&(scp->peer), 0, sizeof(scp->peer));
	}

	if (to) {
		memcpy(&(scp->to), to, sizeof(struct timeval));
	}
	else {
		memset(&(scp->to), 0, sizeof(scp->to));
	}
	
	/* read control */
	scp->pending_read_control.want_len = -1;

	/* write control */
	DBL_INIT_LIST_HEAD(&(scp->waiting_write_control.sending_list_header));

	/* reconn control */
	scp->reconn_control.conn_in_progress = 0;
	scp->reconn_control.last_written_data_len = -1;
	scp->reconn_control.last_last_written_data_len = -1;
	scp->reconn_control.reconn_ms = reconn_ms;

	/* cb control */
	scp->per_conn_cbs.tcp_conn_timeout.user_callback_func = NULL;
	scp->per_conn_cbs.tcp_conn_timeout.earg = NULL;
	
	scp->per_conn_cbs.tcp_terminate.user_callback_func = NULL;
	scp->per_conn_cbs.tcp_terminate.earg = NULL;
	
	scp->per_conn_cbs.data_arrive.user_callback_func = NULL;
	scp->per_conn_cbs.data_arrive.earg = NULL;

	scp->use_time = g_current_time;
	scp->last_active = g_current_time;
	
	return 0;
}

static inline int
_make_tcp_listen_slot(es_conn_context *scp, int port, int fd, struct sockaddr_in *addrp, eservice_manager_t *mgr, struct event_base *ebase)
{
	return __make_slot(scp, port, fd, es_socket_type_tcp_listen, addrp, mgr, NULL, ebase, -1, NULL);
}

static inline int
_make_udp_server_slot(es_conn_context *scp, int port, int fd, struct sockaddr_in *addrp, eservice_manager_t *mgr, struct event_base *ebase)
{
	return __make_slot(scp, port, fd, es_socket_type_udp, addrp, mgr, NULL, ebase, -1, NULL);
}

static inline int
_make_tcp_conn_slot(es_conn_context *scp, int port, int fd, struct sockaddr_in *addrp, eservice_manager_t *mgr, struct sockaddr_in *peerp, struct event_base *ebase)
{
	return __make_slot(scp, port, fd, es_socket_type_tcp_conn, addrp, mgr, peerp, ebase, -1, NULL);
}

static inline int
_make_tcp_client_slot(es_conn_context *scp, int fd, struct sockaddr_in *peerp, int reconn_ms, struct timeval *to, int type)
{
	return __make_slot(scp, 0, fd, type, NULL, &g_es_manager, peerp, g_es_manager.ebase, reconn_ms, to);
}

static int
__listen_bind_group(eservice_manager_t *mgr, eservice_main_conf_t *confp, struct event_base *ebase, int reuseport)
{
	int res = -255;
	es_conn_context *a_esp = NULL;
	struct sockaddr_in addr;
	uint32_t i, j;
	char *ptr = NULL, *endp = NULL;
	long int conv;
	uint16_t port;
	struct evconnlistener *listenp = NULL;
	
	if (!confp)
		return eservice_errcode_param_invalid;

	/* listener */
	for (i = 0; i < confp->bind_group_num; i++) {
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		ptr = NULL;
		endp = NULL;
		conv = 0;
		port = 0;
		
		if (strlen(confp->bind_group[i].device) >= 2) {
			res = get_addr_from_device(confp->bind_group[i].device, &(addr.sin_addr.s_addr));
			if (res < 0) {
				g_warn("get_addr_from_device() failed: %d", res);
				return res;
			}
		}
		else if (strlen(confp->bind_group[i].ip) >= 4) {
			addr.sin_addr.s_addr = inet_addr(confp->bind_group[i].ip);
		}
		else {
			g_warn("invalid device and ip, device: %s, ip: %s", confp->bind_group[i].device, confp->bind_group[i].ip);
			return -1;
		}

		ptr = confp->bind_group[i].port_str;
		endp = ptr;
		while (*ptr) {
			a_esp = NULL;
			
			ptr = endp;
			while (isspace(*ptr) || *ptr == ' ' || *ptr == ',')
				ptr++;

			if (!*ptr)
				break;
			
			conv = strtol(ptr, &endp, 10);
			if (conv < 0 || conv > 65535) {
				g_warn("invalid port: %s\n", ptr);
				return -2;
			}
			else if (conv == 0) {
				break;
			}

			port = (uint16_t)conv;
			addr.sin_port = htons(port);

			g_msg("bind address: [%d] %s:%u", confp->bind_group[i].proto_type, inet_ntoa(addr.sin_addr), port);

			a_esp = __find_empty_slot();
			if (!a_esp) {
				g_warn("__find_empty_slot() failed");
				return -3;
			}

			if (confp->bind_group[i].proto_type == SOCK_STREAM) {
				listen_fd_proto_array[listen_fd_num] = SOCK_STREAM;
				listen_fd_array[listen_fd_num] = create_tcp_listen_socket_struct(&addr, 1, reuseport);
				if (listen_fd_array[listen_fd_num] < 0)
					abort();
				
				res = _make_tcp_listen_slot(a_esp, port, listen_fd_array[listen_fd_num], &addr, mgr, ebase);
				listen_esp_array[listen_fd_num] = a_esp;
				port_statistic_array[listen_fd_num].used = 1;
				port_statistic_array[listen_fd_num].port = a_esp->related_port;
				
				listen_fd_num++;
				
				a_esp->last_event = EV_READ | EV_PERSIST;
				a_esp->ev = event_new(ebase, a_esp->fd, EV_READ | EV_PERSIST, es_listenfd_read_cb, a_esp);
			}
			else if (confp->bind_group[i].proto_type == SOCK_DGRAM) {
				listen_fd_proto_array[listen_fd_num] = SOCK_DGRAM;
				listen_fd_array[listen_fd_num] = create_udp_listen_socket_struct(&addr, 1, reuseport);
				res = _make_udp_server_slot(a_esp, port, listen_fd_array[listen_fd_num], &addr, mgr, ebase);
				listen_esp_array[listen_fd_num] = a_esp;
				port_statistic_array[listen_fd_num].used = 1;
				port_statistic_array[listen_fd_num].port = a_esp->related_port;
				
				listen_fd_num++;
				
				a_esp->last_event = EV_READ | EV_PERSIST;
				a_esp->ev = event_new(ebase, a_esp->fd, EV_READ | EV_PERSIST, es_udpfd_event_cb, a_esp);
			}
			else {
				g_warn("unknown proto type: %d", confp->bind_group[i].proto_type);
				res = -4;
			}

			if (res < 0) {
				g_warn("__make_slot() failed: %d", res);
				return res;
			}

			if (!(a_esp->ev)) {
				g_warn("event_new() failed: %s", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				return -5;
			}

			event_priority_set(a_esp->ev, es_high_priority);
			
			if (confp->listen_timeout > 0) {
				__reset_event_timeout(&(a_esp->to));
				event_add(a_esp->ev, &(a_esp->to));
			}
			else {
				event_add(a_esp->ev, NULL);
			}
	
#if 0
			//listenp = evconnlistener_new_bind(ebase, NULL, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_DISABLED | LEV_OPT_DEFERRED_ACCEPT, -1, (const struct sockaddr *)&addr, sizeof(addr));
			listenp = evconnlistener_new_bind(ebase, NULL, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (const struct sockaddr *)&addr, sizeof(addr));
			if (!listenp) {
				g_warn("evconnlistener_new_bind() failed: %s", strerror(errno));
				return -3;
			}

			evconnlistener_set_cb(listenp, eservice_accept_echo_cb, NULL);
			evconnlistener_set_error_cb(listenp, eservice_accept_error_cb);
			evconnlistener_enable(listenp);
#endif
		}
	}

	return 0;
}

void
es_assign_plugin_log(void)
{
	ASSIGN_PLUGIN_LOG(g_albert_logp, g_log_level);
}

void
es_assign_frame_log(void)
{
	ASSIGN_FRAME_LOG(g_albert_logp, g_log_level);
}

static inline struct eservice_buf_t *
es_get_and_new_readbuf_from_context(es_conn_context *sp)
{
	if (!sp)
		return NULL;

	ES_INVALID_CONN_RET(sp, NULL);
	
	if (!(sp->pending_read_control.buf)) {
		sp->pending_read_control.buf = eservice_buf_new(0);
		sp->pending_read_control.want_len = -1;
	}
	
	return sp->pending_read_control.buf;
}

static inline struct eservice_buf_t *
es_get_readbuf_from_context(es_conn_context *sp)
{
	if (!sp)
		return NULL;
	
	return sp->pending_read_control.buf;
}


static inline ssize_t
es_get_wantlen_from_context(es_conn_context *sp)
{
	if (!sp || !(sp->pending_read_control.buf))
		return -1;

	return sp->pending_read_control.want_len;
}

static inline int
es_set_wantlen_from_context(es_conn_context *sp, ssize_t want_len)
{
	if (!sp)
		return -1;
	
	sp->pending_read_control.want_len = want_len;

	return 0;
}

static inline int
es_set_close_after_next_write(es_conn_context *sp)
{
	if (!sp)
		return 0;

	sp->pending_read_control.close_after_next_write = 1;
	
	return 0;
}

/* 2 for connection refused, 1 for connected, 0 for not yet, -1 for error. */
static int
es_tcp_connect_struct(int *fd_ptr, struct sockaddr_in *sa)
{
	int made_fd = 0;

	if (!fd_ptr)
		return -1;

	*fd_ptr = create_tcp_client_socket();
	made_fd = 1;
	if (set_nonblock_fd(*fd_ptr) < 0) {
		goto err;
	}
				
	set_recvbuf_size(*fd_ptr, ES_TCP_RECVBUF_SIZE);
	set_sendbuf_size(*fd_ptr, ES_TCP_SENDBUF_SIZE);
	set_no_tw(*fd_ptr);

	if (connect_tcp_socket_struct(*fd_ptr, sa) < 0) {
		if (errno == EINTR || errno == EINPROGRESS)
			return 0;
		
		if (errno == ECONNREFUSED)
			return 2;
		
		goto err;
	} else {
		return 1;
	}

err:
	if (made_fd) {
		close_socket(*fd_ptr);
	}
	
	return -1;
}

static int
es_set_tcp_connfd_to_event(es_conn_context *connect_esp)
{
	int res = -255;
	struct timeval *top = g_all_connfd_timeout_p;
	
	if (connect_esp->type == es_socket_type_tcp_client || connect_esp->type == es_socket_type_tcp_client_once) {
		if (connect_esp->to.tv_sec != 0 || connect_esp->to.tv_usec != 0) {
			top = &(connect_esp->to);
		}
	}
	
	if (event_add(connect_esp->ev, top) < 0) {
		g_warn("event_add() failed: %s", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
		return -2;
	}
	
	return 0;
}

static int
es_add_tcp_connfd_to_event(es_conn_context *connect_esp, short flag)
{
	int res = -255;

	ES_INVALID_CONN_RET(connect_esp, -1);

	if (connect_esp->ev) {
		event_free(connect_esp->ev);
		connect_esp->ev = NULL;
	}
	
	connect_esp->last_event = flag;
	connect_esp->ev = event_new(connect_esp->ebase, connect_esp->fd, flag, es_fd_event_cb, connect_esp);
	if (!(connect_esp->ev)) {
		g_warn("event_new() failed: %s", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
		return -1;
	}

	event_priority_set(connect_esp->ev, es_middle_priority);
	
	return es_set_tcp_connfd_to_event(connect_esp);
}

static int
es_set_udp_connfd_to_event(es_conn_context *connect_esp)
{
	int res = -255;
	
	if (event_add(connect_esp->ev, g_all_connfd_timeout_p) < 0) {
		g_warn("event_add() failed: %s",evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
		return -2;
	}
	
	return 0;
}

static int
es_add_udp_connfd_to_event(es_conn_context *connect_esp, short flag)
{
	int res = -255;

	ES_INVALID_CONN_RET(connect_esp, -1);

	if (connect_esp->ev) {
		event_free(connect_esp->ev);
		connect_esp->ev = NULL;
	}
	
	connect_esp->last_event = flag;
	connect_esp->ev = event_new(connect_esp->ebase, connect_esp->fd, flag, es_fd_event_cb, connect_esp);
	if (!(connect_esp->ev)) {
		g_warn("event_new() failed: %s", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
		return -1;
	}

	event_priority_set(connect_esp->ev, es_middle_priority);
	
	return es_set_udp_connfd_to_event(connect_esp);
}

static void
es_listenfd_read_cb(int fd, short flag, void *arg)
{
	int connfd, i = 0, res = -255;
	struct timeval timeout;
	es_conn_context *conn_sp = NULL, *sp = (es_conn_context *)arg;
	socklen_t peer_len = 0;
	struct sockaddr_in peer;
	uint32_t j;
	
	if (flag & EV_TIMEOUT) {
		//g_dbg("listen timeout, fd: %d", fd);

		if ((sp->egr) && (sp->egr->cbs.listen_timeout.user_callback_func)) {
			CALL_USER_PLUGIN(res, sp->egr->cbs.listen_timeout.user_callback_func, sp->egr, NULL, NULL, sp->egr->garg);
			if (res < 0) {
				g_warn("user mod listen timeout cb failed: %d", res);
			}
		}
	}
	else if (flag & EV_READ) {
		for (i = 0; i < ES_TCP_MAX_ACCEPT_ONCE; i++) {
			peer_len = sizeof(peer);
			connfd = accept(fd, (struct sockaddr *)&(peer), &peer_len);
			if (connfd >= 0) {
				g_dbg("listen acceptable, listenfd: %d, connfd: %d, peer address: %s, peer port: %u", fd, connfd, inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));

				for (j = 0; j < ES_TCP_MAX_LISTEN_PORT; j++) {
					if (port_statistic_array[j].used) {
						if (port_statistic_array[j].port == sp->related_port) {
							port_statistic_array[j].recv_num++;
						}
					}
					else {
						break;
					}
				}
				
				if (set_nonblock_fd(connfd) < 0) {
					g_warn("set nonblock failed: %s, fd: %d", strerror(errno), connfd);
					close_socket(connfd);
					continue;
				}
				
				set_recvbuf_size(connfd, ES_TCP_RECVBUF_SIZE);
				set_sendbuf_size(connfd, ES_TCP_SENDBUF_SIZE);
				set_dontfragment(connfd, 0);
				//set_maxseg_size(connfd, 1200);
				set_no_tw(connfd);

				if(accept_fd_num >= g_es_main_conf.acceptfd_limit)
                {
                    g_dbg("system over load close fd: %d, self_accept_fd_num:%d", connfd,shm_ptr[shm_index]);
                    close(connfd);//send RST
                    continue;
                }

                sockaddr_in self_sock;
                peer_len = sizeof(self_sock);
                getsockname(connfd, (struct sockaddr *)&self_sock, &peer_len);
                g_dbg("new conn: local sock addr %s", inet_ntoa(self_sock.sin_addr));

				conn_sp = __find_empty_slot();
				if (!conn_sp) {
					g_warn("find_empty_slot() failed");
					close_socket(connfd);
					continue;
				}

				_make_tcp_conn_slot(conn_sp, sp->related_port, connfd, &self_sock, sp->egr, &peer, sp->ebase);

				res = es_add_tcp_connfd_to_event(conn_sp, EV_READ | EV_PERSIST);
				if (res < 0) {
					g_warn("es_add_tcp_connfd_to_event() failed: %d", res);
					close_socket(connfd);
					return;
				}
				/*add to lru list*/
				if (sp->type == es_socket_type_tcp_listen){
					DBL_LIST_ADDT(&(conn_sp->lru_node.list), &(g_tail_lru_node.list));
					g_lru_num ++;
				}
				
				g_dbg(" conn sp: %p, seq: %u, fd: %d  sp->type :%d lru_node: %p", conn_sp, conn_sp->seq, conn_sp->fd, sp->type, &(conn_sp->lru_node.list));
				
				if ((sp->egr) && (sp->egr->cbs.after_accept.user_callback_func)) {
					CALL_USER_PLUGIN(res, sp->egr->cbs.after_accept.user_callback_func, conn_sp->egr, conn_sp->linked_eup, NULL, conn_sp->egr->garg);
					if (res < 0) {
						g_warn("user mod after accept cb failed: %d", res);
					}
				}
			}
			else {
				if (errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK) {
					g_warn("accept() failed: %s", strerror(errno));
				}

				break;
			}
		}
	}
	else {
		g_warn("unknown flag: %x", flag);
	}

	return;
}

/* first law: avoid copying */
static void
es_fd_event_cb(int fd, short flag, void *arg)
{
	int res = -255;
	int need_call_user = 0;
	es_conn_context *sp = (es_conn_context *)arg;
	ssize_t nread = 0, want_len = 0, tot_len = 0;
	int clean_flag = 0;
	socklen_t socklen = 0;
	
	if (fd != sp->fd) {
		g_warn("fd mismatch, readable fd: %d, context fd: %d, sp: %p", fd, sp->fd, sp);
		if (sp->fd == -1) {				/* maybe we just close it for reconn */
			return;
		}
		else {
			_free_tcp_context(sp);
		}
		
		return;
	}
	
	if (flag & EV_TIMEOUT) {
		//g_dbg("fd timeout, fd: %d", fd);
		if (sp->per_conn_cbs.tcp_conn_timeout.user_callback_func) {
			CALL_USER_PLUGIN(res, sp->per_conn_cbs.tcp_conn_timeout.user_callback_func, sp->egr, sp->linked_eup, sp->per_conn_cbs.tcp_conn_timeout.earg, sp->egr->garg);
		}
		else if ((sp->egr) && (sp->egr->cbs.tcp_conn_timeout.user_callback_func)) {
			CALL_USER_PLUGIN(res, sp->egr->cbs.tcp_conn_timeout.user_callback_func, sp->egr, sp->linked_eup, NULL, sp->egr->garg);
			if (res < 0) {
				g_warn("user mod conn timeout cb failed: %d", res);
			}
		}
	}
	/* EV_WRITE must before EV_READ, to send uncompletely data */
	else if (flag & EV_WRITE) {
		if (sp->reconn_control.conn_in_progress > 0) {
			g_dbg("fd connect init writable, fd: %d", fd);
			sp->reconn_control.conn_in_progress = 0;

			/* reset connection info */
			socklen = sizeof(struct sockaddr_in);
			getsockname(fd, (struct sockaddr *)(&(sp->related_native)), &socklen);
		}
		else {
			g_dbg("fd writeable, fd: %d", fd);

			ES_UPDATE_CONN_ACTIVE(sp, g_current_time);
			if (sp->type == es_socket_type_tcp_conn)
			{
				DBL_LIST_DEL(&(sp->lru_node.list));
				DBL_LIST_ADDT(&(sp->lru_node.list), &(g_tail_lru_node.list));
			}
		}
		
		es_conn_context_write(sp);
	}
	else if (flag & EV_READ) {
		if (sp->reconn_control.conn_in_progress > 0) {
			g_dbg("fd connect init readable, fd: %d", fd);
			sp->reconn_control.conn_in_progress = 0;

			/* reset connection info */
			socklen = sizeof(struct sockaddr_in);
			getsockname(fd, (struct sockaddr *)(&(sp->related_native)), &socklen);
		}
		else {
			g_dbg("fd readable, fd: %d", fd);

			if (sp->type == es_socket_type_tcp_conn)
			{
				DBL_LIST_DEL(&(sp->lru_node.list));
				DBL_LIST_ADDT(&(sp->lru_node.list), &(g_tail_lru_node.list));
			}

		}

		ES_UPDATE_CONN_ACTIVE(sp, g_current_time);
		
do_read_data:	
		while (1) {
			clean_flag = 0;
			want_len = es_get_wantlen_from_context(sp);
			g_dbg("want_len: %zd", want_len);
			
			 if (want_len == 0) {
				g_warn("invalid want len: %zd", want_len);
				clean_flag = 1;
				goto out_process;
			}
			
			nread = eservice_buf_read(fd, es_get_and_new_readbuf_from_context(sp), want_len);
			if (nread < 0) {
				if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
					g_dbg("read eagain, fd: %d", fd);

					clean_flag = 0;
					goto out_process;
				}
				else {
					g_msg("read failed: %s, fd: %d", strerror(errno), fd);
					
					clean_flag = 1;
					goto out_process;
				}
			}
			else if (nread == 0) {			/* terminated */
				g_msg("read close, fd: %d", fd);
				
				clean_flag = 1;
				goto out_process;
			}
			else {
				g_dbg("read len: %zd, fd: %d", nread, fd);

				need_call_user = 0;
				if (want_len < 0) {
					need_call_user = 1;
				}
				else {
					want_len -= nread;
					if (want_len <= 0) {
						need_call_user = 1;
						es_set_wantlen_from_context(sp, -1);
					}
					else {
						need_call_user = 0;
						es_set_wantlen_from_context(sp, want_len);
					}
				}

				if (need_call_user > 0) {
					while (1) {
						ssize_t after_len = 0;
						
						tot_len = es_get_readbuf_from_context(sp) ? (eservice_buf_datalen(es_get_readbuf_from_context(sp))) : (0);
						if (sp->per_conn_cbs.data_arrive.user_callback_func) {
							CALL_USER_PLUGIN(res, sp->per_conn_cbs.data_arrive.user_callback_func, sp->egr, sp->linked_eup, sp->per_conn_cbs.data_arrive.earg, sp->egr->garg);
						}
						else{
							CALL_USER_PLUGIN(res, sp->egr->cbs.data_arrive.user_callback_func, sp->egr, sp->linked_eup, NULL, sp->egr->garg);
						}

						if (ES_INVALID_CONN(sp)) {
							g_dbg("connection not exists, sp: %p", sp);
							return;
						}
						
						after_len = eservice_buf_datalen(es_get_readbuf_from_context(sp));
						g_dbg("res:%d,after_len:%d,tot_len:%d",res,after_len,tot_len);
						if (res != eservice_cb_data_arrive_ok || after_len == 0 || after_len == tot_len)
							break;
					}
				}
				else {
					res = eservice_cb_data_arrive_not_enough;
				}
				
				switch (res) {
					case eservice_cb_data_arrive_fatal:
						g_warn("fatal error, close fd: %d", fd);
						/* fall */
					case eservice_cb_data_arrive_finish:
						clean_flag = 1;

						goto out_process;

					case eservice_cb_data_arrive_close:
						clean_flag = 0;
						es_set_close_after_next_write(sp);
						_close_read_tcp_context(sp);

						goto out_process;

					case eservice_cb_data_arrive_not_enough:
					case eservice_cb_data_arrive_ok:
							/* go next read */
							break;
							
					default:
						g_warn("invalid return val: %d", res);
						clean_flag = 1;

						goto out_process;
				}
			}
		}
	}
	else {
		g_warn("unknown flag: %x", flag);
		clean_flag = 1;
		goto out_process;
	}

out_process:
	if (ES_INVALID_CONN(sp)) {
		g_dbg("connection not exists, sp: %p", sp);
		return;
	}
	
	if (clean_flag == 1) {
		if (sp->per_conn_cbs.tcp_terminate.user_callback_func) {
			CALL_USER_PLUGIN(res, sp->per_conn_cbs.tcp_terminate.user_callback_func, sp->egr, sp->linked_eup, sp->per_conn_cbs.tcp_terminate.earg, sp->egr->garg);
		}
		else if ((sp->egr) && (sp->egr->cbs.tcp_terminate.user_callback_func)) {
			CALL_USER_PLUGIN(res, sp->egr->cbs.tcp_terminate.user_callback_func, sp->egr, sp->linked_eup, NULL, sp->egr->garg);
			if (res < 0) {
				g_warn("user mod conn terminate cb failed: %d", res);
			}
		}

		if (ES_INVALID_CONN(sp)) {
			g_dbg("connection not exists, sp: %p", sp);
			return;
		}
		
		if (sp->type == es_socket_type_tcp_client) {
			_close_tcp_context(sp);
			__clean_read_control(sp);

			/* set reconn timer */
			if (es_timer_reconnect(sp) < 0) {
				_free_tcp_context(sp);
			}
		}
		else {
			_free_tcp_context(sp);
		}
	}
	
	return;
}

static void
es_udpfd_event_cb(int fd, short flag, void *arg)
{
	int res = -255;
	int need_call_user = 0;
	es_conn_context *sp = (es_conn_context *)arg;
	ssize_t nread = 0, want_len = 0, tot_len = 0;
	int clean_flag = 0;
	
	if (fd != sp->fd) {
		g_warn("fd mismatch, readable fd: %d, context fd: %d", fd, sp->fd);
		_free_tcp_context(sp);
		return;
	}
	
	if (flag & EV_TIMEOUT) {
		//g_dbg("udp fd timeout, fd: %d", fd);
	}
	/* EV_WRITE must before EV_READ, to send uncompletely data */
	else if (flag & EV_WRITE) {
		
		g_dbg("fd writeable, fd: %d", fd);
		
		ES_UPDATE_CONN_ACTIVE(sp, g_current_time);
		
		es_conn_context_write(sp);
	}
	else if (flag & EV_READ) {
		g_dbg("udp fd readable, fd: %d", fd);

		ES_UPDATE_CONN_ACTIVE(sp, g_current_time);
		
do_read_data:	
		while (1) {
			clean_flag = 0;
			want_len = es_get_wantlen_from_context(sp);
			g_dbg("want_len: %zd", want_len);
			
			 if (want_len >= 0) {
				g_dbg("udp not support this want len: %zd", want_len);
				want_len = -1;
				es_set_wantlen_from_context(sp, want_len);
			}
			
			nread = eservice_buf_read(fd, es_get_and_new_readbuf_from_context(sp), want_len);
			if (nread < 0) {
				if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
					g_dbg("udp read eagain, fd: %d", fd);

					clean_flag = 0;
					goto out_process;
				}
				else {
					g_warn("udp read failed: %s, fd: %d", strerror(errno), fd);
					
					clean_flag = 1;
					goto out_process;
				}
			}
			else if (nread == 0) {
				g_msg("udp read close, fd: %d", fd);

				clean_flag = 1;
				goto out_process;
			}
			else {
				g_dbg("udp read len: %zd, fd: %d", nread, fd);

				while (1) {
					ssize_t after_len = 0;
						
					tot_len = es_get_readbuf_from_context(sp) ? (eservice_buf_datalen(es_get_readbuf_from_context(sp))) : (0);
					if (sp->per_conn_cbs.data_arrive.user_callback_func) {
						CALL_USER_PLUGIN(res, sp->per_conn_cbs.data_arrive.user_callback_func, sp->egr, sp->linked_eup, sp->per_conn_cbs.data_arrive.earg, sp->egr->garg);
					}
					else{
						CALL_USER_PLUGIN(res, sp->egr->cbs.data_arrive.user_callback_func, sp->egr, sp->linked_eup, NULL, sp->egr->garg);
					}

					if (ES_INVALID_CONN(sp)) {
						g_dbg("connection not exists, sp: %p", sp);
						return;
					}
					
					after_len = eservice_buf_datalen(es_get_readbuf_from_context(sp));

					if (res != eservice_cb_data_arrive_ok || after_len == 0 || after_len == tot_len)
						break;
				}
				
				clean_flag = 0;
				switch (res) {
					case eservice_cb_data_arrive_not_enough:
					case eservice_cb_data_arrive_ok:
						/* go next read */
						break;
						
					case eservice_cb_data_arrive_fatal:
						g_warn("fatal error, close fd: %d", fd);					
						/* fall */
					case eservice_cb_data_arrive_finish:
						/* fall */
					case eservice_cb_data_arrive_close:
						/* fall */
					default:
						clean_flag = 1;
						goto out_process;
				}
			}
		}
	}
	else {
		g_warn("unknown flag: %x", flag);
		clean_flag = 1;
		goto out_process;
	}

out_process:
	if (clean_flag == 1) {
		_clean_udp_context(sp);		
	}
	
	return;
}

int
es_reconnect_tcp_context(es_conn_context *sp)
{
	int res = -255;
	
	if (!sp || (sp->type != es_socket_type_tcp_client && sp->type != es_socket_type_tcp_client_once)) {
		return -1;
	}

	ES_INVALID_CONN_RET(sp, -1);
	
	g_dbg("reconnect for %s:%u", inet_ntoa(sp->peer.sin_addr), ntohs(sp->peer.sin_port));
	
	close_socket(sp->fd);
	__clean_read_control(sp);

	sp->use_time = g_current_time;

	res = es_tcp_connect_struct(&(sp->fd), &(sp->peer));
	if (res == 2) {		/* connect refused */
		g_dbg("connect %s:%u refused", inet_ntoa(sp->peer.sin_addr), ntohs(sp->peer.sin_port));
		return es_timer_reconnect(sp);
	}
	else if (res == 1) {		/* ok */
		es_add_tcp_connfd_to_event(sp, EV_READ | EV_PERSIST);
		return 1;
	}
	else if (res == 0) {		/* pending */
		sp->reconn_control.conn_in_progress = 1;
		return es_add_tcp_connfd_to_event(sp, EV_READ | EV_WRITE | EV_PERSIST);
	}
	else {
		g_warn("connect %s:%u failed", inet_ntoa(sp->peer.sin_addr), ntohs(sp->peer.sin_port));
		return es_timer_reconnect(sp);
	}
	
	return 0;
}

static int
__es_conn_context_write_udp(es_conn_context *sp)
{
	int res = -255;
	struct dbl_list_head *nodep = NULL;
	struct eservice_buf_t *ebp = NULL;
	struct evbuffer *evp = NULL;
	int ebp_data = 0;
	ssize_t cur_ebp_written = 0;
	
	g_dbg("want send udp data, sp: %p, num: %u", sp, sp->waiting_write_control.n_to_write);

	if (sp->fd < 0) {
		goto do_schedual_rw;
	}
	
	if (sp->waiting_write_control.n_to_write == 0)
		goto do_schedual_rdonly;

	while (1) {
		ebp = NULL;
		evp = NULL;
		nodep = NULL;

		if (sp->waiting_write_control.n_to_write == 0) {
			goto do_schedual_rdonly;
		}
		
		sp->waiting_write_control.n_to_write--;
		DBL_LIST_DEL_FIRST(nodep, &(sp->waiting_write_control.sending_list_header));
		if (!nodep) {
			g_warn("confuse node");
			sp->waiting_write_control.n_to_write = 0;
			goto do_schedual_rdonly;
		}

		ebp = DBL_LIST_ENTRY(nodep, struct eservice_buf_t, list);
		evp = ESERVICE_ESBUF2EVBUF(ebp);

		cur_ebp_written = 0;
		
		/* time out ? */
		if (g_current_time > eservice_buf_get_usetime(ebp) + ES_DEFAULT_WRITE_TIMEOUT) {
			g_warn("timeout ebp: %p, use time: %ld, now: %ld", ebp, eservice_buf_get_usetime(ebp), g_current_time);

			eservice_buf_free(ebp);
			ebp = NULL;
			evp = NULL;
			nodep = NULL;

			continue;
		}
		
		while (1) {
			ebp_data = evbuffer_get_length(evp);
			if (ebp_data == 0) {
				eservice_buf_free(ebp);
				ebp = NULL;
				evp = NULL;
				nodep = NULL;
				break;
			}
			
			res = evbuffer_write_atmost(evp, sp->fd, -1);
			g_dbg("write ebp: %p, tot data: %u, actual len: %d", ebp, ebp_data, res);
			if (res < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					goto do_schedual_rw;
				}
				else	{
					g_warn("write data failed: %s", strerror(errno));
					goto do_error;
				}
			} 
			else if (res == 0) {
				/* eof case
			   	XXXX Actually, a 0 on write doesn't indicate
			   	an EOF. An ECONNRESET might be more typical.
			 	*/

				g_warn("write data reset");
				res = -ECONNRESET;
				goto do_error;
			}
			else {
				cur_ebp_written += res;
			}
		}
	}

do_schedual_rw:
	g_dbg("rw schedule, fd: %d", sp->fd);
	
	if (nodep) {
		if (cur_ebp_written > 0) {
			if (ebp->send_failed_cb) {
				CALL_USER_BUF_PLUGIN(res, ebp->send_failed_cb, sp->linked_eup, ebp, ebp->arg);
			}
				
			eservice_buf_free(ebp);

		}
		else {
			DBL_LIST_ADDT(nodep, &(sp->waiting_write_control.sending_list_header));		/* push it back at tail */
		}

		ebp = NULL;
		evp = NULL;
		nodep = NULL;
	}
	
	if (es_add_udp_connfd_to_event(sp, EV_READ | EV_WRITE) < 0) {
		goto do_error;
	}

	return 0;
	
do_schedual_rdonly:
	g_dbg("rd schedule, fd: %d", sp->fd);

	if (sp->last_event & EV_WRITE) {
		if (es_add_udp_connfd_to_event(sp, EV_READ | EV_PERSIST) < 0) {
			goto do_error;
		}
	}
	else {
		if (es_set_udp_connfd_to_event(sp) < 0) {
			goto do_error;
		}
	}
	
	return 0;
	
do_error:
	if (ebp) {
		eservice_buf_free(ebp);
		ebp = NULL;
	}
	
	_clean_udp_context(sp);
	
	return res;
}

static int
__es_conn_context_write_tcp(es_conn_context *sp)
{
	int res = -255;
	struct dbl_list_head *nodep = NULL;
	struct eservice_buf_t *ebp = NULL;
	struct evbuffer *evp = NULL;
	int write_once = 0, ebp_data = 0, reconn = 0;
	ssize_t total_written_num = 0, cur_ebp_written = 0;
	
	g_dbg("want send data, sp: %p, num: %u", sp, sp->waiting_write_control.n_to_write);

	if (sp->fd < 0) {
		reconn = 1;
		goto do_schedual_rw;
	}
	
	if (sp->waiting_write_control.n_to_write == 0)
		goto do_schedual_rdonly;

	reconn = 0;
	while (1) {
		ebp = NULL;
		evp = NULL;
		nodep = NULL;

		if (sp->waiting_write_control.n_to_write == 0) {
			goto do_schedual_rdonly;
		}
		
		sp->waiting_write_control.n_to_write--;
		DBL_LIST_DEL_FIRST(nodep, &(sp->waiting_write_control.sending_list_header));
		if (!nodep) {
			g_warn("confuse node");
			sp->waiting_write_control.n_to_write = 0;
			goto do_schedual_rdonly;
		}

		ebp = DBL_LIST_ENTRY(nodep, struct eservice_buf_t, list);
		evp = ESERVICE_ESBUF2EVBUF(ebp);
		cur_ebp_written = 0;
		
		/* time out ? */
        /*
		if (g_current_time > eservice_buf_get_usetime(ebp) + ES_DEFAULT_WRITE_TIMEOUT) {
			g_warn("timeout ebp: %p, use time: %ld, now: %ld", ebp, eservice_buf_get_usetime(ebp), g_current_time);

			eservice_buf_free(ebp);
			ebp = NULL;
			evp = NULL;
			nodep = NULL;

			continue;
		}
        */
		
		while (1) {
			ebp_data = evbuffer_get_length(evp);
			if (ebp_data == 0) {
				eservice_buf_free(ebp);
				ebp = NULL;
				evp = NULL;
				nodep = NULL;
				break;
			}
			
			write_once = (ebp_data > ES_TCP_SENDBUF_SIZE) ? ES_TCP_SENDBUF_SIZE : (-1);
			
			res = evbuffer_write_atmost(evp, sp->fd, write_once);
			g_dbg("write ebp: %p, tot data: %u, want len: %d, actual len: %d", ebp, ebp_data, write_once, res);
			if (res < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					goto do_schedual_rw;
				}
				else	{
					g_warn("write data failed: %s", strerror(errno));
					
					if (sp->type == es_socket_type_tcp_client || sp->type == es_socket_type_tcp_client_once) {
						reconn = 1;
						goto do_schedual_rw;
					}

					goto do_error;
				}
			} 
			else if (res == 0) {
				/* eof case
			   	XXXX Actually, a 0 on write doesn't indicate
			   	an EOF. An ECONNRESET might be more typical.
			 	*/

				g_warn("write data reset");
				if (sp->type == es_socket_type_tcp_client) {
					reconn = 1;
					goto do_schedual_rw;
				}

				res = -ECONNRESET;
				goto do_error;
			}
			else {
				cur_ebp_written += res;
				total_written_num += res;
			}
		}
	}

do_schedual_rw:
	g_dbg("rw schedule, fd: %d", sp->fd);
	
	if (nodep) {
		if (cur_ebp_written > 0) {
			if (reconn) {
				g_warn("data not complete, but need reconnect, sp: %p, ebp: %p, left len: %zu", 
                       sp, ebp, evbuffer_get_length(evp));
#if 0				/* XXX FIXME: Should be this way, but the evbuffer_write_atmost doesn't reserve the data already written, so we just notice the user. */
				if (ebp->retry_num == 0) {		/* this ebp maybe cause the peer shut the connection, so we abandom it. */
					if (ebp->send_failed_cb) {
						user_eup->sp = sp;
						CALL_USER_BUF_PLUGIN(res, ebp->send_failed_cb, &user_eup, ebp, ebp->arg);
					}
					
					eservice_buf_free(ebp);
				}
				else {
					ebp->retry_num--;
					DBL_LIST_ADDH(nodep, &(sp->waiting_write_control.sending_list_header));		/* push it back at head */
				}
#else
				if (ebp->send_failed_cb) {
					CALL_USER_BUF_PLUGIN(res, ebp->send_failed_cb, sp->linked_eup, ebp, ebp->arg);
				}
					
				eservice_buf_free(ebp);
#endif
			}
			else {
				/* data write not completely */
				g_dbg("data not complete, waiting for next writable."
                      "sp: %p, ebp: %p, left len: %zu", 
                      sp, ebp, evbuffer_get_length(evp));
				DBL_LIST_ADDH(nodep, &(sp->waiting_write_control.sending_list_header));
				sp->waiting_write_control.n_to_write++;
			}
		}
		else {
			g_msg("nothing written at all"
                  "sp: %p, ebp: %p, left len: %zu", 
                  sp, ebp, evbuffer_get_length(evp));
			DBL_LIST_ADDH(nodep, &(sp->waiting_write_control.sending_list_header));
			sp->waiting_write_control.n_to_write++;
		}

		ebp = NULL;
		evp = NULL;
		nodep = NULL;
	}
	
	if (reconn) {
		sp->reconn_control.last_last_written_data_len = sp->reconn_control.last_written_data_len;
		sp->reconn_control.last_written_data_len = total_written_num;
		total_written_num = 0;

		if (sp->reconn_control.conn_in_progress == 1) {
			g_msg("reconncting, ignore");
		}
		else {
			if (sp->reconn_control.last_written_data_len != 0 || sp->reconn_control.last_last_written_data_len != 0) {
				g_warn("reconnect for %s:%u now", inet_ntoa(sp->peer.sin_addr), ntohs(sp->peer.sin_port));
				res = es_reconnect_tcp_context(sp);
				if (res < 0) {
					goto do_error;
				}
			}
			else {
				g_warn("reconnect for %s:%u interval", inet_ntoa(sp->peer.sin_addr), ntohs(sp->peer.sin_port));
				close_socket(sp->fd);
				__clean_read_control(sp);
				res = es_timer_reconnect(sp);
				if (res < 0) {
					goto do_error;
				}
			}
		}
	}
	else {
#if 1		/* maybe EV_PERSIST is optional */
		if (es_add_tcp_connfd_to_event(sp, EV_READ | EV_WRITE | EV_PERSIST) < 0) 
#else
		if (es_add_tcp_connfd_to_event(sp, EV_READ | EV_WRITE) < 0)
#endif
		{
			goto do_error;
		}
	}

	return 0;
	
do_schedual_rdonly:
	g_dbg("rd schedule, fd: %d", sp->fd);

	if (sp->pending_read_control.close_after_next_write > 0) {			/* close */
		g_dbg("write ok, close, sp: %p", sp);
		res = 0;
		goto do_error;
	}
	
	if (sp->last_event & EV_WRITE) {
		if (es_add_tcp_connfd_to_event(sp, EV_READ | EV_PERSIST) < 0) {
			goto do_error;
		}
	}
	else {
		if (es_set_tcp_connfd_to_event(sp) < 0) {
			goto do_error;
		}
	}
	
	return 0;
	
do_error:
	if (ebp) {
		eservice_buf_free(ebp);
		ebp = NULL;
	}
	
	if (sp->type == es_socket_type_tcp_client) {
		g_warn("client tcp conn reach error, sp: %p", sp);
		_close_tcp_context(sp);
	}
	else {
		_free_tcp_context(sp);
	}
	
	return res;
}

static void
__es_reconn_event_cb(int fd, short flag, void *arg)
{
	int res = -255;
	es_conn_context *sp = NULL;

	if (!arg)
		return;

	sp = (es_conn_context *)arg;
	
	if (ES_INVALID_CONN(sp))
		return;
	
	if (sp->reconn_control.conn_in_progress == 0)
		return;

	g_warn("reconnect for %s:%u", inet_ntoa(sp->peer.sin_addr), ntohs(sp->peer.sin_port));
	res = es_reconnect_tcp_context(sp);
	if (res < 0) {
		_free_tcp_context(sp);
	}

	return;
}

int
es_conn_context_write(es_conn_context *sp)
{
	if (!sp) {
		g_warn("invalid param, sp: %p, fd: %d", sp, sp->fd);
		return -1;
	}

	ES_INVALID_CONN_RET(sp, -1);
	
	if (sp->type == es_socket_type_udp) {
		return __es_conn_context_write_udp(sp);
	}
	else {
		return __es_conn_context_write_tcp(sp);
	}
	
	return -1;		/* dummy */
}

int 
es_timer_reconnect(es_conn_context *sp)
{
	struct timeval tv;
	 	
	if (!sp || ES_INVALID_CONN(sp))
		return -1;
	
	if (sp->reconn_control.reconn_ms <= 0)
		return -2;
	
	msec2timeval(&tv, (long)(sp->reconn_control.reconn_ms));
	
	if (!(sp->reconn_control.reconn_timerev)) {
		sp->reconn_control.reconn_timerev = evtimer_new(sp->ebase, __es_reconn_event_cb, (void *)sp);
	}

	if (sp->reconn_control.reconn_timerev) {
		if (evtimer_add(sp->reconn_control.reconn_timerev, &tv) < 0) {
			g_warn("timer reconn failed, sp: %p", sp);
			return -2;
		}
	}
	else {
		return -3;
	}

	sp->reconn_control.conn_in_progress = 1;			/* We are waiting for reconnecting. */
	return 0;
}

es_conn_context *
__es_create_tcp(struct sockaddr_in *sa, int reconn_ms, struct timeval *to, int type)
{
	es_conn_context *sp = NULL;
	int res = -255;
	
	sp = __find_empty_slot();
	if (!sp) {
		g_warn("find_empty_slot() failed");
		return NULL;
	}

	_make_tcp_client_slot(sp, -1, sa, reconn_ms, to, type);

	res = es_reconnect_tcp_context(sp);
	if (res < 0) {
		_free_tcp_context(sp);
		return NULL;
	}
	
	return sp;
}

es_conn_context *
es_create_tcp_conn(struct sockaddr_in *sa, int reconn_ms, struct timeval *to)
{
	return __es_create_tcp(sa, reconn_ms, to, es_socket_type_tcp_client);
}

es_conn_context *
es_create_tcp_imme(struct sockaddr_in *sa, int reconn_ms, struct timeval *to)
{
	return __es_create_tcp(sa, reconn_ms, to, es_socket_type_tcp_client_once);
}

int
es_destroy_tcp_conn(es_conn_context *sp)
{
	if (sp) {
		return _free_tcp_context(sp);
	}

	return -1;
}

int
__es_set_signal(eservice_manager_t *mgr, event_base *ebase)
{
	/* mask SIGPIPE signal */
	sigpipe_event = evsignal_new(ebase, SIGPIPE, __sigpipe_callback, NULL);
	if (evsignal_add(sigpipe_event, NULL) < 0) {
		g_warn("evsignal_add() failed: %s\n", strerror(errno));
        exit(2);
	}

    /* mask SIGUSR1 signal for reload config */
    sigusr1_event = evsignal_new(ebase, SIGUSR1, __sigusr1_callback, mgr);
    if (evsignal_add(sigusr1_event, NULL) < 0) {
        g_warn("evsingal_add(sigusr1_event) failed: %s\n", strerror(errno));
        exit(2);
    }
    return 0;
}

int
__es_set_timer(event_base *ebase)
{
	struct timeval one_sec, s30_sec;
	/* 1s timer */
	one_sec_timer_event = evtimer_new(ebase, __timer1sec_callback, NULL);
	one_sec.tv_sec = 1;
	one_sec.tv_usec = 0;
	if (evtimer_add(one_sec_timer_event, &one_sec) < 0) {
		g_warn("evtimer_add() failed: %s\n", strerror(errno));
        return -1;
	}

	/* 1s timer */
	s30_sec_timer_event = evtimer_new(ebase, __timer30sec_callback, NULL);
	s30_sec.tv_sec = 30;
	s30_sec.tv_usec = 0;
	if (evtimer_add(s30_sec_timer_event, &s30_sec) < 0) {
		g_warn("evtimer_add() failed: %s\n", strerror(errno));
		return -1;
	}
	/* make all connfd timeout */
	g_all_connfd_timeout_p = (struct timeval *)event_base_init_common_timeout(ebase, &g_tcp_conn_timeout);
    return 0;
}

int
__es_set_shm()
{
	shm_ptr = (uint32_t*)mmap(NULL,sizeof(uint32_t)*ESERVICE_MAX_WORKER_NUM,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	if(shm_ptr == MAP_FAILED)
	{
	    g_warn("mmap failed: %s", strerror(errno));
	    exit(4);
	}
	g_warn("mmap successful");
    memset(&(shm_ptr[0]),0,sizeof(uint32_t) * ESERVICE_MAX_WORKER_NUM);
    return 0;
}

int
__es_set_spe_listen(eservice_manager_t *mgr, event_base *ebase, int timeout)
{
    struct sockaddr_in spe_addr;
    es_conn_context *spe_esp = NULL;
    int spe_fd = -1;
    in_addr_t the_eth1_ip;

    /* process special listen */
    spe_esp = __find_empty_slot();
    if (!spe_esp) {
        g_warn("__find_empty_slot() failed");
        return -10;
    }

    int res;
    res = get_addr_from_device("eth1", &the_eth1_ip);
    if (res < 0) {
        g_warn("get_addr_from_device() failed: %d", res);
        return -11;
    }

    process_spe_listen_ip = (network32_t)the_eth1_ip;
    process_spe_listen_port = ES_TCP_PROCESS_SPE_LISTEN_PORT_BASE + shm_index;

    memset(&spe_addr, 0, sizeof(spe_addr));
    spe_addr.sin_family = AF_INET;	
    spe_addr.sin_addr.s_addr = the_eth1_ip;
    spe_addr.sin_port = htons(process_spe_listen_port);
    spe_fd = create_tcp_listen_socket_struct(&spe_addr, 1, 0);
    if (spe_fd < 0)
        abort();

    res = _make_tcp_listen_slot(spe_esp, process_spe_listen_port, spe_fd, &spe_addr, mgr, ebase);

    spe_esp->last_event = EV_READ | EV_PERSIST;
    spe_esp->ev = event_new(ebase, spe_esp->fd, EV_READ | EV_PERSIST, es_listenfd_read_cb, spe_esp);
    if (timeout > 0) {
        __reset_event_timeout(&(spe_esp->to));
        event_add(spe_esp->ev, &(spe_esp->to));
    }
    else {
        event_add(spe_esp->ev, NULL);
    }

    return 0;
}

int
__es_reset_log(eservice_main_conf_t *confp, int num)
{
	char child_log_path[1024];
	char child_logic_so_log[1024];

    /* open frame log */
    g_es_frame_log_level = confp->log_level;
    free_dbg(g_es_frame_logp);
    g_es_frame_logp = new CRollLog();
    if (!g_es_frame_logp) {
        g_err("init frame log failed");
    }

    snprintf(child_log_path, sizeof(child_log_path) - 1, "%s_%d", confp->network_log_path, num);
    g_es_frame_logp->init(string(child_log_path), "", 1, 20*(1<<20), 10);               /* micro second */
    //g_es_frame_logp->init(string(child_log_path), "", 0, 20*(1<<20), 10);             /* second */


    /* open plugin log */
    if (strlen(confp->logic_so_log) <= 1) {
        g_dbg("no user plugin log, use the frame log");
        g_es_user_plugin_log_level = g_es_frame_log_level;
        g_es_user_plugin_logp = g_es_frame_logp;
    }
    else {
        g_es_user_plugin_log_level = confp->logic_so_log_level;
        free_dbg(g_es_user_plugin_logp);
        g_es_user_plugin_logp = new CRollLog();
        if (!g_es_user_plugin_logp) {
            g_err("init plugin log failed");
        }

        snprintf(child_logic_so_log, sizeof(child_logic_so_log) - 1, "%s_%d", confp->logic_so_log, num);
        g_es_user_plugin_logp->init(string(child_logic_so_log), "", 1, 20*(1<<20), 10);             /* micro second */
        //g_es_user_plugin_logp->init(string(child_logic_so_log), "", 0, 20*(1<<20), 10);               /* second */

        ASSIGN_PLUGIN_LOG(g_albert_logp, g_log_level);
        g_warn("========== plugin log start ==========\n");
    }
    return 0;
}

int
__es_readd_listen_event(eservice_manager_t *mgr, event_base *ebase, int timeout)
{
    int res = 0;
    /* readd ev */
    for (uint32_t j = 0; j < listen_fd_num; j++) {
        /* listen fd */
        listen_esp_array[j]->ebase = ebase;
        if (listen_fd_proto_array[j] == SOCK_STREAM) {
            listen_esp_array[j]->last_event = EV_READ | EV_PERSIST;
            listen_esp_array[j]->ev = event_new(ebase, listen_fd_array[j], EV_READ | EV_PERSIST, es_listenfd_read_cb, listen_esp_array[j]);
        }
        else if (listen_fd_proto_array[j] == SOCK_DGRAM) {
            listen_esp_array[j]->last_event = EV_READ | EV_PERSIST;
            listen_esp_array[j]->ev = event_new(ebase, listen_fd_array[j], EV_READ | EV_PERSIST, es_udpfd_event_cb, listen_esp_array[j]);
        }
        else {
            g_warn("unknown proto type: %d", listen_fd_proto_array[j]);
            res = -4;
        }

        if (res < 0) {
            g_warn("__make_slot() failed: %d", res);
            return res;
        }

        if (!(listen_esp_array[j]->ev)) {
            g_warn("event_new() failed: %s", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
            return -5;
        }

        event_priority_set(listen_esp_array[j]->ev, es_high_priority);

        if (timeout > 0) {
            //__reset_event_timeout(&(listen_esp_array[j]->to));
            event_add(listen_esp_array[j]->ev, &(listen_esp_array[j]->to));
        }
        else {
            event_add(listen_esp_array[j]->ev, NULL);
        }
    }
    return 0;
}

int
eservice_network_run(eservice_manager_t *mgr, eservice_main_conf_t *confp)
{
	int res = -255;
	struct event_base *ebase;
	uint32_t i;
	uint32_t j;
	
	/* Redirect all Libevent log messages to the my log. */
	event_set_log_callback(__write_to_mylog_cb);

	__set_resource(confp);
	
#ifdef	DEBUG
	event_enable_debug_mode();
#endif
	
	/* init context */
	res = __init_global_context(mgr, confp);
	if (res < 0) {
		g_warn("init_global_context() failed: %d", res);
		exit(1);
	}

    /*
     * 1. get ebase
     * 2. shm
     * 3. bind 80s
     * -------- we test SO_REUSEPORT here
     * -------- yes, fork, then bind
     * -------- no, just bind, then fork 
     * 4. bind 17000 + i
     * 5. signal
     * 6. timer
     *
     */

	ebase = __get_strongest_event_backend();
	if (!ebase) {
		g_warn("__get_strongest_event_backend() failed");
		exit(1);
	}
	mgr->ebase = ebase;

    __es_set_shm();

    int reuseport = test_tcp_listen_socket_reuseport();
    g_warn("%s SO_REUSEPORT", reuseport ? "support" : "unsupport");

    if (reuseport == 0)
    {
        res = __listen_bind_group(mgr, confp, ebase, reuseport);
        if (res != 0)
        {
            g_warn("__listen_bind_group() failed: %s", strerror(errno));
            exit(1);
        }
    }

    shm_index = 0;
    for (i = 0; i < confp->process_num -1; ++i)
    {
        res = fork();
        if (res < 0)
        {
            g_warn("fork() failed: %s", strerror(errno));
            exit(1);
        }

        if (res == 0) // child
        {
            __es_reset_log(confp, i + 1);
            g_msg("fork a child %d, pid: %d", i + 1, getpid());

            /* maybe event_reinit(ebase); */
            event_base_free(ebase);
            ebase = __get_strongest_event_backend();
            if (!ebase) {
                g_warn("__get_strongest_event_backend() failed");
                exit(1);
            }
            mgr->ebase = ebase;

            if (reuseport == 0)
            {
                /* 重新绑定listen event到新的ebase */
                res = __es_readd_listen_event(mgr, ebase, confp->listen_timeout);
                if (res != 0)
                {
                    g_warn("__es_readd_listen_event failed: %d", res);
                    exit(1);
                }
            }

            shm_index = i+1;
            break;
        }
    }

    if (reuseport)
    {
        res = __listen_bind_group(mgr, confp, ebase, reuseport);
        if (res != 0)
        {
            g_warn("__listen_bind_group() failed: %s", strerror(errno));
            exit(1);
        }
    }
    
    res = __es_set_spe_listen(mgr, ebase, confp->listen_timeout);
    if (res != 0) return res;

    __es_set_signal(mgr, ebase);
    __es_set_timer(ebase);

	/* call user mod init */
	CALL_USER_PLUGIN(res, mgr->cbs.mod_init.user_callback_func, mgr, NULL, NULL, mgr->garg);
	if (res < 0) {
		g_warn("user mod init failed: %d", res);
		exit(1);
	}

	g_dbg("user plugin init success: %d", g_log_level);

	/* call user mod prerun */
	res = 0;
	if (mgr->cbs.pre_run.user_callback_func) {
		CALL_USER_PLUGIN(res, mgr->cbs.pre_run.user_callback_func, mgr, NULL, NULL, mgr->garg);
		if (res < 0) {
			g_warn("user mod pre run failed: %d", res);
		}
	}
	


	while (1) {
		if (now_terminate > 0) {
			g_warn("now terminated");
			
			if (ebase)
				event_base_free(ebase);

			exit(0);
		}

		event_base_loop(ebase, EVLOOP_ONCE);
		
		shm_ptr[shm_index] = accept_fd_num;
		clear_lru();

		//sleep(1);
		//g_dbg("tick");
	}

	return 0;
}

