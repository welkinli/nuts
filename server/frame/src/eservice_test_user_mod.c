#ifndef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"TEST_PLUGIN"
#endif

#include	"eservice_user.h"
#include	"common_albert.h"
#include	"common_private_protocol.h"
#include	"program_utils.h"

#define	TEST_MAX_INVALID_PKG_LEN	8192

extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;

static struct eservice_unit_t *my_eup = NULL;
static struct eservice_unit_t *last_client_eup = NULL;

static int 
pkg_send_failed_cb(eservice_unit_t *eup, eservice_buf_t *ebp, void *arg)
{
	g_warn("=> pkg_send_failed_cb(), eup: %p, ebp: %p, arg: %p", eup, ebp, arg);

	return 0;
}

static eservice_callback_return_val 
my_conn_data_arrive_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	struct eservice_unit_t *client_eup = NULL;
	struct eservice_buf_t *data_pkg = NULL;
	int res = -255;
	
	g_dbg("=> my_conn_data_arrive_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	while (1) {
		if (eservice_user_datalen(eup) <= 0)
			break;
		
		res = eservice_user_pop_inbuf(eup, -1, &data_pkg);
		if (res < 0 || !data_pkg) {
			g_warn("eservice_user_pop_inbuf() failed: %d", res);
			break;
		}

		eservice_user_push_outbuf(last_client_eup, data_pkg);
	}
	
	return eservice_cb_data_arrive_ok;
}

static eservice_callback_return_val 
my_conn_terminate_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	g_warn("=> my_conn_terminate_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);
	
	return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_init(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	g_dbg("=> eservice_cb_init(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_pre_run(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	struct timeval to = {3, 0};
	
	g_dbg("=> eservice_cb_pre_run(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	my_eup = eservice_user_register_tcp_conn(NULL, "127.0.0.1", "12345", 1000, &to);
	if (!my_eup) {
		g_warn("eservice_user_register_tcp_conn() failed");
		exit(1);
	}

	eservice_user_set_tcp_conn_data_arrive_cb(my_eup, my_conn_data_arrive_cb, NULL);
	eservice_user_set_tcp_conn_terminate_cb(my_eup, my_conn_terminate_cb, NULL);
	return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_after_accept(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	g_dbg("=> eservice_cb_after_accept(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_listen_timeout(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	g_dbg("=> eservice_cb_listen_timeout(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	return eservice_cb_success;
}

#if 0
extern "C" 
eservice_callback_return_val eservice_cb_tcp_conn_timeout(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	g_dbg("=> eservice_cb_tcp_conn_timeout(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	return eservice_cb_success;
}
#endif

extern "C" 
eservice_callback_return_val eservice_cb_tcp_terminate(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	g_dbg("=> eservice_cb_tcp_terminate(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_data_arrive(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
	struct eservice_buf_t *pkg_data = NULL;
	char *ptr = NULL;
	uint32_t pkg_len = 0;
	ssize_t buf_data_len = 0;
	common_private_protocol_header_V2 *headp = NULL;
	int res = -255;
	
	g_dbg("=> eservice_cb_data_arrive(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

	while (1) {
		pkg_data = NULL;
		pkg_len = 0;
		
		buf_data_len = eservice_user_datalen(eup);
		if (buf_data_len < 0) {
			return eservice_cb_data_arrive_fatal;
		}
		
		if (buf_data_len < (ssize_t)(sizeof(common_private_protocol_header_V2))) {
			eservice_user_next_need(eup, sizeof(common_private_protocol_header_V2) - buf_data_len);
			return eservice_cb_data_arrive_not_enough;
		}

		headp = (common_private_protocol_header_V2 *)eservice_user_start(eup);

		pkg_len = NETWORK2HOST_32(headp->length);
		g_dbg("pkg len: %u", pkg_len);

		if (pkg_len > TEST_MAX_INVALID_PKG_LEN)
			pkg_len = 128;

		if (buf_data_len < (ssize_t)pkg_len) {
			eservice_user_next_need(eup, pkg_len - buf_data_len);
			return eservice_cb_data_arrive_not_enough;
		}
		
		res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
		if (res < 0 || !pkg_data) {
			g_warn("eservice_user_pop_inbuf() failed: %d", res);
			return eservice_cb_data_arrive_fatal;
		}

		eservice_buf_set_control(pkg_data, 3, pkg_send_failed_cb, NULL);
		res = eservice_user_push_outbuf(my_eup, pkg_data);
		if (res < 0) {
			g_warn("eservice_user_push_outbuf() failed: %d", res);
			eservice_buf_free(pkg_data);
		}

		last_client_eup = eup;
	}	
	
	return eservice_cb_data_arrive_ok;
}
