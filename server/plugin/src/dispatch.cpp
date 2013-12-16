/*
 * dispatch.cpp
 *
 *  Created on: 2013-12-8
 */

#ifndef DISPATCH_CPP_
#define DISPATCH_CPP_

#include "plugin.h"
#include "hiredis.h"
#include "async.h"
#include "redis_libevent.h"

static std::map<uint64_t, void *> dispatch_long_peer_map;
static volatile struct redisContext *g_redis_context, *g_redis_context_slave;
static struct timeval g_redis_timeout =
{ 1, 500000 }; // 1.5 seconds

static eservice_callback_return_val dispatch_long_conn_data_arrive_cb(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	struct eservice_buf_t *pkg_data = NULL;
	ssize_t pkg_len = 0, buf_data_len = 0;
	int res = -255;
	plugin_task *the_tsp = NULL;
	wns_inner_psy_header_t *psy_headp = NULL;
	struct eservice_buf_t *the_ebp = NULL;
	struct eservice_unit_t *the_eup[DEFAULT_MAX_GROUP_USER_NUM];
	char http_length_buffer[MAX_LINE_LEN];
	int snlen = 0;
	struct dbl_list_head *dptr = NULL;
	std::string errmsg;

	g_dbg("=> dispatch_long_conn_data_arrive_cb(), eup: %p, earg: %p, garg: %p",
			eup, earg, garg);

	while (1)
	{
		pkg_data = NULL;
		pkg_len = 0;
		psy_headp = NULL;

		buf_data_len = eservice_user_datalen(eup);
		if (buf_data_len < 0)
		{
			g_warn(
					"dispatch_long_conn_data_arrive_cb eservice_user_datalen < 0 ");
			return eservice_cb_data_arrive_fatal;
		}

		if (buf_data_len < (ssize_t) sizeof(wns_inner_psy_header_t)
				|| buf_data_len > WNS_ACC_MAX_VALID_PKG_LEN)
		{
			return eservice_cb_data_arrive_not_enough;
		}

		/* parse protocol */
		psy_headp = (wns_inner_psy_header_t *) (eservice_user_pos_ex(eup, 0,
				(ssize_t) (sizeof(wns_inner_psy_header_t))));
		if (psy_headp == NULL)
		{
			g_warn("eservice_user_pos_ex return NULL");
			return eservice_cb_data_arrive_fatal;
		}

		if (psy_headp->SOH != WNS_INNER_SIGN)
		{
			g_warn("unknown protocol: %#0x", psy_headp->SOH);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL,
					NULL, NULL, 1);
			return eservice_cb_data_arrive_fatal;
		}

		pkg_len = ntohl(psy_headp->Len);
		g_msg("wns inner protocol, len: %zd", pkg_len);

		if (pkg_len <= (ssize_t) sizeof(wns_inner_psy_header_t)
				|| pkg_len > WNS_ACC_MAX_VALID_PKG_LEN)
		{
			g_warn("invalid wns inner pkg len: %zd", pkg_len);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL,
					NULL, NULL, 1);
			return eservice_cb_data_arrive_fatal;
		}

		if (buf_data_len < pkg_len)
		{
			eservice_user_next_need(eup, pkg_len - buf_data_len);
			return eservice_cb_data_arrive_not_enough;
		}

		res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
		if (res < 0 || !pkg_data)
		{
			g_warn("eservice_user_pop_inbuf() failed: %d", res);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL,
					NULL, NULL, 1);
			return eservice_cb_data_arrive_fatal;
		}
		int broadcast_num = 0;
		res = wns_acc_get_proper_async(eup, pkg_data, pkg_len, the_eup,
				&the_tsp, broadcast_num);
		if (res < 0)
		{
			g_msg("invalid async");
			eservice_buf_free(pkg_data);
			continue;
		}

		if (the_tsp && the_tsp->output_ebp_list.number > 0)
		{
			dptr = ss_list_pop_head_struct(&(the_tsp->output_ebp_list));
			if (dptr)
			{
				the_ebp = eservice_buf_list2buf(dptr);
				eservice_buf_set_control(the_ebp, 3, pkg_send_failed_cb, NULL);

				if (the_tsp->protocol_type == 2)
				{ /* add HTTP fake header */
					snlen = snprintf(http_length_buffer,
							sizeof(http_length_buffer) - 1, "%zd\r\n\r\n",
							eservice_buf_datalen(the_ebp));
					eservice_buf_prepend(the_ebp, http_length_buffer, snlen);
					eservice_buf_prepend(the_ebp, http_fake_header,
							http_fake_header_len);
				}

				// 如果是oc点加速过来的,需要加上相应的头
				//prepend_proxy_header(the_tsp, the_ebp);
				struct eservice_buf_t *send_ebp = NULL;
				for (int i = 0; i < broadcast_num; i++)
				{
					send_ebp = eservice_buf_new(0); //这里要用0，否则refer出来的evbuff_drain的时候会报错
					if (eservice_buf_clone_data(send_ebp, the_ebp) < 0)
					{
						eservice_buf_free(send_ebp);
						continue;
					}
					//一个一个找到连接来回传出去，类似广播
					res = eservice_user_push_outbuf(the_eup[i], send_ebp);
					if (res < 0)
					{
						g_msg("eservice_user_push_outbuf() failed: %d", res);
						//wns_acc_logSession(WNS_CODE_ACC_CLIENT_SND_ERR, eup, NULL, NULL, the_tsp);
					}
				}
			}
		}

		wns_acc_free_task_struct(the_tsp);
		the_tsp = NULL;
		dptr = NULL;
	}

	return eservice_cb_data_arrive_ok;
}

static eservice_callback_return_val dispatch_long_conn_terminate_cb(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	g_warn("=> dispatch_long_conn_terminate_cb(), eup: %p, earg: %p, garg: %p",
			eup, earg, garg);

	return eservice_cb_success;
}

static eservice_callback_return_val dispatch_long_conn_timeout_cb(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	//g_dbg("=> dispatch_long_conn_timeout_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);
	return eservice_cb_success;
}

static int get_dispatch_svr(int cmd, dispatch_svr_t *target)
{
	target->setip("172.0.0.1");
	target->port = 0;
	target->type = REDIS_SVR;
	return 0;
}

//redis这里有2个方案
//1.短链接：每次请求都连接，连接中记录client信息，用来在回调中确认是那个请求来的，用完释放，要注意配置fd的reuse，可能会有性能问题
//2.长连接，同步模式
// 这里先简单用同步模式来搞吧
void redis_get_callback(redisAsyncContext *c, void *r, void *privdata)
{
	redisReply *reply = r;
	plugin_task *tsp = privdata;
	if (reply == NULL)
		return;
	printf("argv[%s]: %s\n", (char*) privdata, reply->str);

	/* Disconnect after receiving the reply to GET */
	redisAsyncDisconnect(c);
}

void redis_connect_callback(const redisAsyncContext *c, int status)
{
	if (status != REDIS_OK)
	{
		printf("Error: %s\n", c->errstr);
		return;
	}
	printf("Connected...\n");
}

void redis_disconnect_callback(const redisAsyncContext *c, int status)
{
	if (status != REDIS_OK)
	{
		printf("Error: %s\n", c->errstr);
		return;
	}
	printf("Disconnected...\n");
}

static int redis_register(plugin_task *tsp, dispatch_svr_t peer)
{
	struct event_base *base = tsp->eup->sp->ebase;

	redisAsyncContext *c = redisAsyncConnect(peer.ip, peer.port);
	if (!c || (c && c->err))
	{
		/* Let *c leak for now... */
		g_warn("connect to redis error: %s", c->errstr);
		return -1;
	}
	redisLibeventAttach(c, base);
	redisAsyncSetConnectCallback(c, redis_connect_callback);
	redisAsyncSetDisconnectCallback(c, redis_disconnect_callback);
//
//	redisAsyncCommand(c, redis_get_callback, NULL, "SET key %b", argv[argc - 1],
//				strlen(argv[argc - 1]));
	/*
	 redisAsyncCommand(c, redis_get_callback, (char*)"end-1", "GET key");
	 */
	//event_base_dispatch(base);
	return 0;
}
static struct eservice_unit_t *
plugin_find_proper_dispatch_and_send(plugin_task *tsp,
		struct eservice_buf_t *send_ebp)
{
	struct timeval to =
	{ 1, 0 };
	dispatch_svr_key_t key;
	struct eservice_unit_t *dispatch_long_eup = NULL;
	std::map<uint64_t, void *>::iterator it;

	dispatch_svr_t target;
	if (get_dispatch_svr(tsp->plugin_head->Cmd, &target) < 0)
		return NULL; //TODO
	key.ip_port.ip = inet_addr(target.ip);
	key.ip_port.port = (uint32_t) (target.port);

	it = dispatch_long_peer_map.find(key.key64);
	if (it == dispatch_long_peer_map.end())
	{
		//连接不存在，重新创建
		g_dbg("new conn for %s:%u, key: %lu,cmd:%d", target.ip, target.port,
				key.key64, tsp->plugin_head->Cmd);
		//下面的代码应该很少会用到,除非有自建server
		if (!dispatch_long_eup)
		{
			dispatch_long_eup = eservice_user_register_tcp_conn(NULL, target.ip,
					&target.port, 1000, &to);
			if (!dispatch_long_eup)
			{
				g_warn("eservice_user_register_tcp_conn() failed");
				return NULL;
			}

			g_dbg("new dispatch connection: %p cmd:%d", dispatch_long_eup,
					tsp->plugin_head->Cmd);

			eservice_user_set_tcp_rw_buffer_size(dispatch_long_eup,
					2 * 1024 * 1024, 2 * 1024 * 1024); /* 2M */
			eservice_user_set_tcp_conn_data_arrive_cb(dispatch_long_eup,
					dispatch_long_conn_data_arrive_cb,
					(void *) dispatch_long_eup);
			eservice_user_set_tcp_conn_terminate_cb(dispatch_long_eup,
					dispatch_long_conn_terminate_cb,
					(void *) dispatch_long_eup);
			eservice_user_set_tcp_conn_timeout_cb(dispatch_long_eup,
					dispatch_long_conn_timeout_cb, (void *) dispatch_long_eup,
					3000); /* 3s timeout */

			dispatch_long_peer_map[key.key64] = dispatch_long_eup;
		}

	}
	else
	{
		//连接还存在
		g_dbg("reuse conn for %s:%u, key: %lu,cmd:%d", target.ip, target.port,
				key.key64, tsp->plugin_head->Cmd);
		dispatch_long_eup = (struct eservice_unit_t *) it->second;
		if (eservice_user_eup_is_invalid(dispatch_long_eup))
		{
			g_warn("dispatch connection invalid, eup: %p,cmd:%d",
					dispatch_long_eup, tsp->plugin_head->Cmd);
			dispatch_long_eup = NULL;
		}
		if (send_ebp)
		{
			res = eservice_user_push_outbuf(dispatch_long_eup, send_ebp);
			if (res < 0)
			{
				//TODO
			}
		}
		return dispatch_long_eup;
	}
}

int plugin_connect_to_dispatch_and_send_data(eservice_unit_t *eup,
		plugin_task *tsp, struct eservice_buf_t *send_ebp)
{
	int res = -255;
	struct timeval to =
	{ 1, 0 };
	struct eservice_unit_t *dispatch_long_eup = NULL;
	dispatch_long_eup = plugin_find_proper_dispatch_and_send(tsp, send_ebp);
	if (!dispatch_long_eup)
	{
		g_warn("wns_acc_find_proper_dispatch() failed");
		return -1;
	}
	return 0;
}

#endif /* DISPATCH_CPP_ */
