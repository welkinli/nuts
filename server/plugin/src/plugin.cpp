#include	"eservice_user.h"
#include	"common_albert.h"
#include	"program_utils.h"
#include "qmf_protocal_inter.h"
#include "comm.h"
#define	MAX_INVALID_PKG_LEN	8192

extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;

static struct eservice_unit_t *my_eup = NULL;
static struct eservice_unit_t *last_client_eup = NULL;

//全局变量
section_info_t *g_section;
//逻辑数据结构
plugin_main_config_t g_plugin_main_config;

#define max_section_num 128

//用户第一次连接上来要返回conf里面配置的section 的信息

static int init_section(int section_num)
{
	if (section_num > max_section_num || max_section_num <= 0)
	{
		return -1;
	}
	g_section = (section_info_t *) malloc(section_num * sizeof(section_info_t));
	//填充section info  数据
	return 0;
}

//后端数据发送失败处理
static int pkg_send_failed_cb(eservice_unit_t *eup, eservice_buf_t *ebp,
		void *arg)
{
	g_warn("=> pkg_send_failed_cb(), eup: %p, ebp: %p, arg: %p", eup, ebp, arg);

	return 0;
}

//主进程在bind和listen之后会执行此函数一次，也可以通过信号来呼起执行此函数，主要进行一些配置的读取和初始化工作 和  eservice_cb_pre_run 没有本质的区别 ,两个可以合并成一个
//1.数据库的ip，账号密码 这里配置
//2.redis的ip端口这里配置
//3.牌局的人数这里配置，一局的最小下注大小，最大下注大小这里配置
//可以通过发送信号来呼起执行此函数，以达到热加载配置的目的
extern "C" eservice_callback_return_val eservice_cb_init(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	g_dbg("=> eservice_cb_init(), eup: %p, earg: %p, garg: %p", eup, earg,
			garg);
	/* init task struct list */
	if (wns_acc_init_task_struct_list(g_wns_acc_main_config.task_num,
			&g_min_wns_acc_tsp_address_val, &g_max_wns_acc_tsp_address_val) < 0)
		g_err("wns_acc_init_task_struct_list() failed");
	return eservice_cb_success;
}

//主进程在bind和listen之后会执行此函数一次
//这里可以进行前期的数据初始化工作: 内存申请和数据结构初始化，链表建立

extern "C" eservice_callback_return_val eservice_cb_pre_run(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{

	return eservice_cb_success;
}

//连接建立之后会执行一次 ，这作为逻辑server使用的话，执行次数会很少 ,暂时没有什么功能需要放在这里
extern "C" eservice_callback_return_val eservice_cb_after_accept(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	g_dbg("=> eservice_cb_after_accept(), eup: %p, earg: %p, garg: %p", eup,
			earg, garg);

	return eservice_cb_success;
}

extern "C" eservice_callback_return_val eservice_cb_listen_timeout(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	g_dbg("=> eservice_cb_listen_timeout(), eup: %p, earg: %p, garg: %p", eup,
			earg, garg);

	return eservice_cb_success;
}

//连接关闭或者断开的情况要执行这里的清理逻辑，这里作为逻辑server 不需要做什么操作
extern "C" eservice_callback_return_val eservice_cb_tcp_terminate(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	g_dbg("=> eservice_cb_tcp_terminate(), eup: %p, earg: %p, garg: %p", eup,
			earg, garg);

	return eservice_cb_success;
}

extern "C" eservice_callback_return_val eservice_cb_tcp_conn_timeout(
		struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg,
		void *garg)
{
	//return eservice_cb_success;
	//cleaneup:
//	std::map<uint64_t, void *>::iterator the_uin_iter = wns_async_uin_map.find(
//			user_context->uin);
//	wns_async_uin_map.erase(the_uin_iter);
	eservice_user_conn_set_user_data(eup, NULL);
	eservice_user_destroy_tcp_conn(eup);
	return eservice_cb_success;
}

//主体逻辑操作，数据过来之后进入这里执行
//先把所有的功能都在这写齐全，到时候再 :分进程-通过管道分发出去分别执行
//后端连接的执行也在这处理
extern "C" eservice_callback_return_val eservice_cb_data_arrive(
		struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg,
		void *garg)
{
	struct eservice_buf_t *pkg_data = NULL;
	char *ptr = NULL;
	uint32_t pkg_len = 0;
	ssize_t buf_data_len = 0;
	QMF_PROTOCAL::QmfAccHead *headp = NULL;
	int res = -255;
	struct eservice_buf_t * the_ebp = NULL;
	eservice_unit_t *client_eup = NULL;
	struct dbl_list_head *dptr = NULL;
	;
	g_dbg("=> eservice_cb_data_arrive(), eup: %p, earg: %p, garg: %p", eup,
			earg, garg);

	while (1)
	{
		pkg_data = NULL;
		pkg_len = 0;

		buf_data_len = eservice_user_datalen(eup);
		if (buf_data_len < 0)
		{
			return eservice_cb_data_arrive_fatal;
		}

		if (buf_data_len < (ssize_t) (sizeof(QMF_PROTOCAL::QmfAccHead)))
		{
			eservice_user_next_need(eup,
					sizeof(QMF_PROTOCAL::QmfAccHead) - buf_data_len);
			return eservice_cb_data_arrive_not_enough;
		}

		headp = (QMF_PROTOCAL::QmfAccHead*) eservice_user_start(eup);

		pkg_len = NETWORK2HOST_32(headp->Len);
		g_dbg("pkg len: %u", pkg_len);

		if (pkg_len > MAX_INVALID_PKG_LEN)
		{
			//数据包错乱，这里会导致缓冲区的多个数据包丢失,可能需要客户端重发请求 //TODO,看看  interface_server 是否对于不回包的情况会重发请求
			g_warn("pkg len: %u,out of MAX_INVALID_PKG_LEN(%d)", pkg_len,
					MAX_INVALID_PKG_LEN);
			return eservice_cb_data_arrive_fatal;
		}

		if (buf_data_len < (ssize_t) pkg_len)
		{
			eservice_user_next_need(eup, pkg_len - buf_data_len);
			return eservice_cb_data_arrive_not_enough;
		}

		res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
		if (res < 0 || !pkg_data)
		{
			g_warn("eservice_user_pop_inbuf() failed: %d", res);
			return eservice_cb_data_arrive_fatal;
		}
		//开始处理业务逻辑
		plugin_task *tsp = plugin_alloc_task_struct();
		if (!tsp)
		{
			g_warn("plugin_alloc_task_struct() failed");
			ESERVICE_DESTROY_EBP(pkg_data);
			continue;
		}
		tsp->eup = eup; //带有数据包的连接信息
		tsp->input_ebp = pkg_data; // 数据包

		int idx = eservice_user_eup_to_index(eup);
		//index 基本等于eservice_unit_t 数组偏移地址
		if (idx < 0)
		{
			g_warn("eservice_user_eup_to_index() failed, eup=%p", eup);
			//ESERVICE_DESTROY_EBP(pkg_data);
			plugin_free_task_struct(tsp);
			continue;
		}
		tsp->client_id = idx;
		tsp->task_id = make_client_id(idx);
		tsp->protocol_type = 1;
		tsp->client_ip = 0; // 这里的clientip是代理的ip，此处未0
		tsp->client_port = 0; //同上 此处为 0
		res = pkg_process(tsp);

////////////////////////////////////////////////////////////////////////////////////////

		//回包
		if (res != 0)
		{
			g_warn("process_pkg() failed: %d", res);
			if (tsp)
			{
				plugin_free_task_struct(tsp);
				return eservice_cb_data_arrive_fatal;
			}
		}
		g_dbg("tsp->output_ebp_list.number:%d", tsp->output_ebp_list.number);
		if (tsp && tsp->output_ebp_list.number > 0)
		{
			dptr = ss_list_pop_head_struct(&(tsp->output_ebp_list));
			if (dptr)
			{
				the_ebp = eservice_buf_list2buf(dptr);
				eservice_buf_set_control(the_ebp, 1, pkg_send_failed_cb, NULL);

				if (client_eup)
				{
					res = eservice_user_push_outbuf(client_eup, the_ebp);
				}
				else
				{
					res = eservice_user_push_outbuf(eup, the_ebp);
				}
				if (res < 0)
				{
					g_warn("eservice_user_push_outbuf() failed: %d", res);
				}
			}
			plugin_free_task_struct(tsp);
			continue;
		}

		g_dbg("tsp->need_not_rsp:%d", tsp->need_not_rsp);
		if (tsp && tsp->need_not_rsp == 0)
		{
			plugin_add_waiting_task_struct(tsp);
		}
		else
		{
			plugin_free_task_struct(tsp);
		}
	}

	//  后端的tcp 操作,只有在连数据库，连redis的时候用到,应该都会用到,粗略如下，也可以参考接入server的分发逻辑
	//	struct timeval to = {3, 0};
	//

	//	my_eup = eservice_user_register_tcp_conn(NULL, "127.0.0.1", "12345", 1000, &to);
	//	if (!my_eup) {
	//		g_warn("eservice_user_register_tcp_conn() failed");
	//		exit(1);
	//	}
	//
	//	eservice_user_set_tcp_conn_data_arrive_cb(my_eup, my_conn_data_arrive_cb, NULL);
	//	eservice_user_set_tcp_conn_terminate_cb(my_eup, my_conn_terminate_cb, NULL);
	return eservice_cb_data_arrive_ok;
}
void fill_rsp_head(plugin_header_t *client_head,
		plugin_respone_head_t *response_head, int error_code)
{
	response_head->head.Cmd = client_head->Cmd;
	response_head->head.Uin = client_head->Uin;
	response_head->head.Key = client_head->Key;
	response_head->head.Ver = client_head->Ver;
	response_head->head.Wns_sign = WNS_INNER_SIGN;
	response_head->head.Len = sizeof(plugin_respone_head_t);
}
int plugin_pkg_error_data(plugin_task *tsp, plugin_header_t *plugin_head,
		const int error_code)
{
	struct dbl_list_head *ptr = NULL, *n = NULL;
	struct eservice_buf_t *send_ebp = NULL;
	plugin_respone_head_t response_head;
	uint8_t busi_buf[MAX_LINE_LEN], pkg_buf[2 * MAX_LINE_LEN];

	/* free output */DBL_LIST_FOR_EACH_SAFE(ptr, n, &(tsp->output_ebp_list.head))
	{
		if (!ptr)
			break;

		send_ebp = eservice_buf_list2buf(ptr);
		ESERVICE_DESTROY_EBP(send_ebp);
	}
	fill_rsp_head(plugin_head, &response_head, error_code);
	response_head.encode();
	ss_list_init_head_struct(&(tsp->output_ebp_list));

	send_ebp = eservice_buf_new(sizeof(plugin_respone_head_t));
	eservice_buf_add(send_ebp, (char*) response_head,
			sizeof(plugin_respone_head_t));

	ss_list_push_head_struct(&(tsp->output_ebp_list),
			eservice_buf_buf2list(send_ebp));
	return 0;
}

int pre_process(uint8_t *data_start, int input_len,
		plugin_header_t *plugin_head)
{
	QMF_PROTOCAL::QmfHead client_head;
	QMF_PROTOCAL::QmfHeadParser m_protocal_parser;
	int logic_head_len = 0;
	QMF_PROTOCAL::QmfAccHead logic_head;
	logic_head.init();
	int res = m_protocal_parser.DecodeQmfAccHead((const char*) data_start,
			(unsigned int) input_len, logic_head);
	if (res != 0)
	{
		char _printhex[256 + 1];
		int _buflen = input_len > 128 ? 128 : input_len;
		HexToStr((const char *) data_start, _buflen, _printhex);
		g_warn("Decode logic_head ERR. ret=%d.  Data=\n%s", res, _printhex);
		return res;
	}
	//解开qmf头
	res = m_protocal_parser.DecodeQmfHead(
			(const char*) data_start
					+ m_protocal_parser.GetAccHeadLen(logic_head),
			(unsigned int) input_len, client_head);
	if (res != 0)
	{
		char _printhex[256 + 1];
		int _buflen = input_len > 128 ? 128 : input_len;
		HexToStr((const char *) data_start, _buflen, _printhex);
		g_warn("Decode client_head ERR. ret=%d.Data=\n%s", res, _printhex);
		return res;
	}
	//	if (client_head.BodyLen == 0)
	//	{
	//		g_warn("receive a NOT-PING package with clientHead.bodylen==0 uin=%lu ",
	//				client_head.Uin);
	//		//包内容为空
	//		return plugin_pkg_error_data(tsp, client_head,-1);
	//	}
	//校验2个该死的包头对不对得上
	if (logic_head.Uin != client_head.Uin || logic_head.Key != client_head.Key
			|| logic_head.Cmd != client_head.Cmd)
	{
		g_warn(
				"receive a package with different uin_server=%lu ,uin_client=%lu",
				logic_head.Uin, client_head.Uin);
		//错误的包
		return -2;
	}
	plugin_head->Cmd = client_head.Cmd;
	plugin_head->Uin = client_head.Uin;
	plugin_head->Key = client_head.Key;
	plugin_head->Ver = client_head.Ver;
	plugin_head->Body = client_head.Body;
	plugin_head->BodyLen = client_head.BodyLen;

}
int pkg_process(plugin_task *tsp)
{
	eservice_unit_t *eup = tsp->eup;
	int res = -1;
	//解开acc头
	plugin_header_t *plugin_head=(plugin_header_t *)malloc(sizeof(plugin_header_t));
	int input_len = eservice_buf_datalen(tsp->input_ebp);
	eservice_buf_make_linearizes(tsp->input_ebp, input_len);
	uint8_t *data_start = eservice_buf_start(tsp->input_ebp);
	tsp->plugin_head = plugin_head;
	if (pre_process(data_start, input_len, plugin_head))
	{
		return plugin_pkg_error_data(tsp, plugin_head, -2);
	}
	//获取包体
	cmd_process(tsp);
}
