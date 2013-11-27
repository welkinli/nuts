#ifndef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"WNS_ACC"
#endif

#include	"wns_es_acc_plugin.h"
#include	"qmf_protocal_define.h"
#include	"pdu_header.h"
#include	"qmf_comm_define.h"
#include	"qmf_protocal_parser.h"
#include	"qmf_head_parser.h"
#include	"wns_errcode.h"
#include	"fileconfig.h"
#include	"singleton.h"
#include	"CQmfCryptor.h"
#include	"CQmfCompress.h"
#include	"comm_dchelper.h"
#include	"qos_client.h"

/************ declare frame defined */
extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;
extern uint32_t g_sac_dcagent_flag;
extern time_t g_current_time;
extern struct timeval g_current_tv;
extern network32_t process_spe_listen_ip;
extern uint16_t process_spe_listen_port;
extern uint32_t evbuff_num ;
extern uint32_t g_max_connection_num;
extern uint64_t g_total_connection_num;
/*****************************/

static const char *special_get_response_str =
"HTTP/1.1 200 OK\r\n"
"Date: Thu, 30 Aug 2012 12:01:27 GMT\r\n"
"Server: Apache/2.2.17 (Unix) PHP/5.3.6\r\n"
"Cache-Control: max-age=1800\r\n"
"Keep-Alive: timeout=5, max=98\r\n"
"Connection: Keep-Alive\r\n"
"Content-Type: text/plain; charset=UTF-8\r\n"
"Content-Length: 17\r\n\r\nlb monitor page\r\n";

static size_t special_get_response_len = 0;

static const char *http_fake_header = 
"HTTP/1.1 200 OK\r\n"
"Server: WNS-ACC/0.1\r\n"
"Content-Type: application/octet-stream\r\n"
"Connection: Keep-Alive\r\n"
"Content-Disposition: attachment; filename=micromsgresp.dat\r\n"
"Content-Length: ";
static size_t http_fake_header_len = 0;

static wns_acc_main_config_t g_wns_acc_main_config;
static ss_list_head_struct wns_acc_task_struct_head; //空闲队列
static ss_list_head_struct wns_acc_task_struct_inuse_list; //使用中队列
static uint32_t wns_acc_task_struct_seq = 1;
static ss_list_head_struct wns_acc_connect_user_context_head;
static ss_list_head_struct wns_acc_connect_user_context_inuse_list;
static uint32_t wns_acc_connect_user_context_seq = 1;
static uint64_t g_min_wns_acc_tsp_address_val = 0;
static uint64_t g_max_wns_acc_tsp_address_val = 0;
static wns_acc_30s_statistic_t wns_30s;
static plugin_user_distribute wns_user_distribute;
static plugin_protocol_statistic_t wns_connection_statisic;
static plugin_idle_statistic_t wns_idle_statisic;
static wns_dynamic_config_t wns_dynamic_config;
static std::map<int, int>wns_acc_valid_appid_map;
static struct eservice_timer_t one_sec_timer;
static struct eservice_timer_t statistic_timer;
static QOSREQUEST qos_dispatch;
static char qos_dispatch_port_str[16];
static wns_acc_30s_push_t push_statistic;

static map<string,string> g_wns_cookie_map;
#ifndef	USE_IMME_TCP
static map<int,std::map<uint64_t, struct eservice_unit_t *> > dispatch_long_eup_map;
#else
static struct eservice_unit_t *dispatch_eup = NULL;
#endif

static network32_t eth1_ip = 0;

static std::map<uint64_t, void *>wns_async_map;

static void wns_acc_free_task_struct(wns_acc_task_struct *tsp);
static void wns_acc_logSession(ENUM_WNS_ERRCODE retcode, eservice_unit_t *eup, QMF_PROTOCAL::QmfAccHead *acc_head, QMF_PROTOCAL::QmfHead *qmf_head, wns_acc_task_struct *tsp, int force_log = 0);
static int wns_make_empty_response(wns_acc_task_struct *tsp, int wns_code);
static int wns_acc_update_failed_L5(QOSREQUEST &qos,int usetime_usec);
static int wns_acc_update_success_L5(QOSREQUEST &qos,int usetime_usec);


static int bit_test(uint32_t const *Base, int Offset) {
    return ((*Base>>Offset)&0x01) == 1;
}

static void
tsp_timeout_process(wns_acc_task_struct *tsp, void *arg)
{
    int res = -255;
    uint32_t i, j;

    if (tsp->client_info_p) {
        struct in_addr the_ip;
        the_ip.s_addr = htonl(tsp->client_info_p->server_ip);

        wns_acc_update_failed_L5(tsp->client_info_p->qos, 1000000*(g_current_time - tsp->use_time));

        g_warn("=> timeout process: %p, arg: %p, use time: %ld, uin: %u, server ip: %s, seq: %u, cmd: %s clientid=%lu", 
               tsp, arg, tsp->use_time, 
               tsp->client_info_p->uin, 
               inet_ntoa(the_ip), 
               tsp->client_info_p->seq, 
               tsp->client_info_p->cmd.c_str(), 
               tsp->client_id);
    }
    else {
        g_warn("=> timeout process: %p, arg: %p, use time: %ld", tsp, arg, tsp->use_time);
    }

    if (tsp->client_id > 0) {
        std::map<uint64_t, void *>::iterator the_iter = wns_async_map.find(tsp->client_id);
        if (the_iter != wns_async_map.end() && the_iter->second != NULL) {
            g_dbg("async tsp timeout, %lu -> %p, eup: %p", 
                  the_iter->first, the_iter->second, 
                  (eservice_user_eup_from_index(get_eup(tsp->client_id))));
            wns_async_map.erase(the_iter);
        }
    }

    /* clog */
    wns_acc_logSession(WNS_CODE_ACC_SERVER_TIMEOUT, NULL, NULL, NULL, tsp);

    if (tsp->cmd_can_retry > 0 && g_wns_acc_main_config.client_retry > 0) {
		wns_make_empty_response(tsp, WNS_CODE_ACC_NEED_RETRY);
    }
	else{
		wns_make_empty_response(tsp, WNS_CODE_ACC_BACKSERVER_ERROR_DATA);
	}

    wns_acc_free_task_struct(tsp);

    return;
}

static int 
tsp_is_timeout(wns_acc_task_struct *tsp, void *arg)
{
    uint32_t timeout = (uint32_t)((uint64_t)arg);

    if (!tsp)
        return 1;

    return g_current_time > (long)(tsp->use_time + timeout);
}

static void
__cond_del_waiting_list(ss_list_head_struct *hsp, int (*need_del)(wns_acc_task_struct *, void *), void *need_del_arg, void (*cleanup)(wns_acc_task_struct *, void *), void *arg)
{
    wns_acc_task_struct *tsp = NULL;
    struct dbl_list_head *ptr = NULL, *next = NULL;

    if (!hsp)
        return;

    DBL_LIST_FOR_EACH_SAFE(ptr, next, &(hsp->head)) {
        if (!ptr)
            break;

        tsp = DBL_LIST_ENTRY(ptr, wns_acc_task_struct, list); 
        if (!need_del || (*need_del)(tsp, need_del_arg)) {
            g_dbg("del tsp: %p, num :%u", ptr, hsp->number);
            DBL_LIST_DEL(ptr);

            tsp->list_position = WNS_ACC_TASK_POS_NOLIST;

            hsp->number--;

            if (cleanup)
                (*cleanup)(tsp, arg);
        }
    }

    return;
}

static void
one_sec_timer_cb(int arg1, short arg2, void *arg)
{
    struct eservice_manager_t *egr = (struct eservice_manager_t *)arg;
    struct timeval one_sec_timer_to = {1, 0};
    int res = -255;

    g_warn("---- task:");
    g_warn("free: %u, waiting: %u evbuff:%d", ss_list_info_head_struct(&wns_acc_task_struct_head), ss_list_info_head_struct(&wns_acc_task_struct_inuse_list), evbuff_num);
    g_warn("-----------");

    __cond_del_waiting_list(&wns_acc_task_struct_inuse_list, tsp_is_timeout, (void *)WNS_ACC_MAX_TSP_PROCESS_SECONDS, tsp_timeout_process, NULL);		/* tsp 10 seconds timeout */

    /* reset timer */
    eservice_user_timer(egr, &one_sec_timer, &one_sec_timer_to, one_sec_timer_cb, egr);
    return;
}

static void
statistic_timer_cb(int arg1, short arg2, void *arg)
{
    struct eservice_manager_t *egr = (struct eservice_manager_t *)arg;
    struct timeval statistic_timer_to = {30, 0};
    int res = -255;
    uint32_t i;

    g_warn("---- statistic:");
    g_warn("req: %u, rsp: %u, disp req: %u, disp rsp: %u, push: %u", wns_30s.req_num, wns_30s.rsp_num, wns_30s.dispatch_req_num, wns_30s.dispatch_rsp_num, wns_30s.push_num);
    for (i = 0; i < WNS_ACC_MAX_LISTEN_PORT; i++) {
        if (wns_30s.port_statistic_array[i].used == 0)
            break;

        g_warn("port: %u, bin prot num: %u, http prot num: %u", wns_30s.port_statistic_array[i].port, wns_30s.port_statistic_array[i].bin_prot_num, wns_30s.port_statistic_array[i].http_prot_num);
    }

    g_warn("---- push statistic:");
    g_warn("total: %u, <30s: %u:%u, <60s: %u:%u, <180s: %u:%u, <300s: %u:%u, <600s: %u:%u, <900: %u:%u, <1200: %u:%u, <1500: %u:%u", push_statistic.push_num, push_statistic.less_30, push_statistic.init_less_30, push_statistic.s_30_60, push_statistic.init_s_30_60, push_statistic.s_60_180, push_statistic.init_s_60_180, push_statistic.s_180_300, push_statistic.init_s_180_300, push_statistic.s_300_600, push_statistic.init_s_300_600, push_statistic.s_600_900, push_statistic.init_s_600_900, push_statistic.s_900_1200, push_statistic.init_s_900_1200, push_statistic.s_1200_1500, push_statistic.init_s_1200_1500);
    g_warn("-----------");

    g_warn("-----------");

    g_warn("---- user distriute statistic:");
    g_warn("user distriute 15~20min:%u, 10~15min:%u, 5~10min:%u, 0~5min:%u" ,
           wns_user_distribute.min15_20_user_ilde_num,
           wns_user_distribute.min10_15_user_ilde_num,
           wns_user_distribute.min5_10_user_ilde_num,
           wns_user_distribute.min0_5_user_ilde_num);

    g_warn("---- ------");

    g_warn("---- clear first_nodata statistic:");
    g_warn("clear first_nodata:%u, commm_idle num:%u, fast_idle_num:%u, highest_idle_num:%u" ,
           wns_idle_statisic.first_no_data_num,
           wns_idle_statisic.comm_idle_time_num,
           wns_idle_statisic.fast_idle_time_num,
           wns_idle_statisic.highest_idle_time_num);
    g_warn("---- ------");

    g_warn("-----------");
    //30s * 120 = 1 hour g_warn once
    g_warn("connections and recived num statistic:");
    g_warn("wns_total conn: %u \n"
           "	wns_tcp_conn:%u [{common_tcp_conn:%u} {ping_tcp_conn :%u}] \n"
           "	wns_thttp_conn:%u [{common_http_conn:%u} {ping_http_conn :%u}] \n",
           wns_connection_statisic.bin_comm_connection_num + wns_connection_statisic.bin_pin_connection_num + 
           wns_connection_statisic.http_comm_connection_num + wns_connection_statisic.http_pin_connection_num,
           wns_connection_statisic.bin_comm_connection_num + wns_connection_statisic.bin_pin_connection_num, 
           wns_connection_statisic.bin_comm_connection_num, wns_connection_statisic.bin_pin_connection_num,
           wns_connection_statisic.http_comm_connection_num + wns_connection_statisic.http_pin_connection_num, 
           wns_connection_statisic.http_comm_connection_num, wns_connection_statisic.http_pin_connection_num);
    g_warn("wns_total_recvied data:%u \n"
           "	{wns_tcp_recvied data:%u} {wns_http_recvied data:%u} \n",
           wns_connection_statisic.bin_client_data_recved + wns_connection_statisic.http_client_data_recved,
           wns_connection_statisic.bin_client_data_recved, wns_connection_statisic.http_client_data_recved
          );
    g_warn("-----------");

    //g_lru_clear_num_30s = 0;
    memset(&wns_user_distribute, 0, sizeof(wns_user_distribute));
    memset(&wns_connection_statisic, 0 ,sizeof(wns_connection_statisic));
    memset(&wns_idle_statisic, 0, sizeof(wns_idle_statisic));
    memset(&wns_30s, 0, sizeof(wns_30s));
    memset(&push_statistic, 0, sizeof(push_statistic));


    /* reset timer */
    eservice_user_timer(egr, &statistic_timer, &statistic_timer_to, statistic_timer_cb, egr);

    return;
}

static int 
pkg_send_failed_cb(eservice_unit_t *eup, eservice_buf_t *ebp, void *arg)
{
    g_warn("=> pkg_send_failed_cb(), eup: %p, ebp: %p, arg: %p", eup, ebp, arg);

    return 0;
}

static int
prepend_proxy_header(wns_acc_task_struct *tsp, eservice_buf_t *ebp)
{
    // 如果是oc点加速过来的,需要加上相应的头
    if (tsp && tsp->client_port != 0)
    {
        wns_cdnproxy_psy_header_t cdn_header;
        cdn_header.Version = 0x82;
        cdn_header.TTL = 32;
        cdn_header.Header_length = (sizeof(cdn_header) + eservice_buf_datalen(ebp));
        cdn_header.User_ip = htonl(tsp->client_ip);
        cdn_header.User_port = htons(tsp->client_port);

        eservice_buf_prepend(ebp, &cdn_header, sizeof(cdn_header));

        g_msg("prepend cdn proxy header");
    }
    return 0;
}

static void HexToStr(const char* lpszInStr, int nInLen, char* lpszHex)
{
    int i, j = 0;
    char l_byTmp;
    for (i = 0; i < nInLen; i++)
    {
        l_byTmp = (lpszInStr[i] & 0xf0) >> 4;
        if (l_byTmp < 10)
            lpszHex[j++] = l_byTmp + '0';
        else
            lpszHex[j++] = l_byTmp - 10 + 'A';
        l_byTmp = lpszInStr[i] & 0x0f;
        if (l_byTmp < 10)
            lpszHex[j++] = l_byTmp + '0';
        else
            lpszHex[j++] = l_byTmp - 10 + 'A';
    }
    lpszHex[j] = 0;

    return;
}

inline static uint64_t get_tick_count(void)
{
    gettimeofday(&g_current_tv, NULL);
    return (uint64_t)(g_current_tv.tv_sec * 1000 + g_current_tv.tv_usec / 1000);
}

static int
wns_acc_init_task_struct_list(uint32_t number, uint64_t *min_val, uint64_t *max_val)
{
    uint32_t i;
    wns_acc_task_struct *tsp = NULL;
    wns_acc_task_struct *task_struct_chunk = NULL;		/* task struct */
    ss_list_head_struct *headp = &wns_acc_task_struct_head;

    dlmalloc_dbg(task_struct_chunk, number * sizeof(wns_acc_task_struct), wns_acc_task_struct*);
    if (!task_struct_chunk) {
        g_warn("malloc() for task struct chunk failed");
        return -2;
    }

    g_msg("task struct linker: DBLIST WITHOUT LOCK");

    ss_list_init_head_struct(headp);
    wns_acc_task_struct_seq = (uint32_t)random();
    if (wns_acc_task_struct_seq == 0)
        wns_acc_task_struct_seq = 1;

    for (i = 0; i < number; i++) {
        tsp = (wns_acc_task_struct *)(&(task_struct_chunk[i]));
        if (!tsp) {			
            g_warn("g_mem_chunk_alloc() for task struct failed");
            return -3;
        }

        DBL_INIT_LIST_HEAD(&(tsp->list));
        ss_list_push_head_struct(headp, &(tsp->list));
        tsp->list_position = WNS_ACC_TASK_POS_FREE_LIST;

        if (i == 0) {
            set_pval(min_val, (uint64_t)tsp);
        }

        if (i == number - 1)
            set_pval(max_val, (uint64_t)tsp);
    }

    ss_list_init_head_struct(&wns_acc_task_struct_inuse_list);
    return 0;
}

static wns_acc_task_struct *
wns_acc_alloc_task_struct(void)
{
    wns_acc_task_struct *tsp = NULL;
    struct dbl_list_head *ptr = NULL;

    ptr = ss_list_pop_head_struct(&wns_acc_task_struct_head);
    if (!ptr)
        return NULL;

    tsp = DBL_LIST_ENTRY(ptr, wns_acc_task_struct, list);

    memset(tsp, 0, sizeof(wns_acc_task_struct));				/* FIXME ugly ! */

    tsp->sign = WNS_ACC_TASK_STRUCT_SIGN;
    tsp->tsp_seq = wns_acc_task_struct_seq;
    wns_acc_task_struct_seq++;
    if (wns_acc_task_struct_seq == 0)
        wns_acc_task_struct_seq = 1;

    ss_list_init_head_struct(&(tsp->output_ebp_list));

    tsp->use_time = g_current_time;
    tsp->list_position = WNS_ACC_TASK_POS_NOLIST;
    tsp->result = -1;
    tsp->need_not_rsp = 0;
    tsp->error_code = 0;
    tsp->cmd_can_retry = 0;
    tsp->client_ip = 0;
    tsp->client_port = 0;
    new_dbg(tsp->client_info_p, client_reserved_info_t);

    g_dbg("### alloc_task_struct(), address: %p ###", tsp);

    return tsp;
}

static void
wns_acc_free_task_struct(wns_acc_task_struct *tsp)
{
    uint32_t i;
    struct dbl_list_head *ptr = NULL, *n = NULL;
    struct eservice_buf_t *ebp = NULL;

    if (!tsp)
        return;

    switch (tsp->list_position) {
    case WNS_ACC_TASK_POS_NOLIST:
        break;

    case WNS_ACC_TASK_POS_WAITING:
        g_msg("free waiting tsp: %p", tsp);
        ss_list_del_head_struct(&wns_acc_task_struct_inuse_list, &(tsp->list));
        break;

    case WNS_ACC_TASK_POS_FREE_LIST:
        g_warn("want free an already freed task struct: %p", tsp);
        ss_list_del_head_struct(&wns_acc_task_struct_head, &(tsp->list));
        return;

    default:
        g_err("invalid list pos: %d", tsp->list_position);
        break;
    }

    if (tsp->already_send_respone == 0) {
        //g_warn("this tsp not reponse: %p", tsp);
    }

    tsp->use_time = 0;

    /* free output */
    DBL_LIST_FOR_EACH_SAFE(ptr, n, &(tsp->output_ebp_list.head)) {
        if (!ptr)
            break;

        ebp = eservice_buf_list2buf(ptr);
        ESERVICE_DESTROY_EBP(ebp);
    }
    ss_list_init_head_struct(&(tsp->output_ebp_list));

    /* free input */
    ESERVICE_DESTROY_EBP(tsp->input_ebp);

    DBL_INIT_LIST_HEAD(&(tsp->list));
    ss_list_push_head_struct(&(wns_acc_task_struct_head), &(tsp->list));
    tsp->list_position = WNS_ACC_TASK_POS_FREE_LIST;

    delete_dbg(tsp->client_info_p);
    tsp->need_not_rsp = 0;

    g_dbg("### free_task_struct(), address: %p ###", tsp);

    return;
}

static void
wns_acc_add_waiting_task_struct(wns_acc_task_struct *tsp)
{
    uint32_t i;

    if (!tsp)
        return;

    if (tsp->list_position != WNS_ACC_TASK_POS_NOLIST) {
        g_warn("invalid pos when add waiting: %d", tsp->list_position);
        return;
    }

    DBL_INIT_LIST_HEAD(&(tsp->list));
    ss_list_push_head_struct(&(wns_acc_task_struct_inuse_list), &(tsp->list));
    tsp->list_position = WNS_ACC_TASK_POS_WAITING;

    return;
}

static void
wns_acc_del_waiting_task_struct(wns_acc_task_struct *tsp)
{
    if (!tsp)
        return;

    if (tsp->list_position != WNS_ACC_TASK_POS_WAITING) {
        g_warn("invalid pos when add waiting: %d", tsp->list_position);
        return;
    }

    ss_list_del_head_struct(&wns_acc_task_struct_inuse_list, &(tsp->list));
    tsp->list_position = WNS_ACC_TASK_POS_NOLIST;

    return;
}

static uint32_t
wns_acc_info_task_struct(void)
{
    return ss_list_info_head_struct(&wns_acc_task_struct_head);
}

static int 
wns_acc_sacagent_check(eservice_unit_t *eup,
                       QMF_PROTOCAL::QmfAccHead *acc_head, 
                       QMF_PROTOCAL::QmfHead *qmf_head,
                       wns_acc_task_struct *tsp)
{
    SAC_QUERTY::WnsSacObj obj;
    int ret = 0;
    obj.appid = 10178; //qzone appid
    obj.setHostIp(ntohl(eth1_ip));

    if(acc_head){
        obj.setUserIp(acc_head->ClientIP);
    }
    //QUA
    if (tsp && tsp->client_info_p) {
        obj.setQua(tsp->client_info_p->qua);
    }
    else {
        string qua = "";
        obj.setQua(qua);
    }
    //cmd
    if (tsp && tsp->client_info_p) {
        obj.servicecmd = tsp->client_info_p->cmd;
    }
    if(strcasecmp(obj.servicecmd.c_str(), "wns.handshake") == 0)
        return 0;

    obj.uin = qmf_head->Uin;
    g_dbg("uin:%u hostip:%s, userip:%s, servicecmd:%s, qua:%s \n", 
          obj.uin, obj.hostip_a.c_str(), obj.userip_a.c_str(), obj.servicecmd.c_str(), obj.qua.c_str());
    ret = SAC_QUERTY::sac_query(obj);
    return ret;

}

static void 
wns_acc_logSession(ENUM_WNS_ERRCODE retcode, eservice_unit_t *eup,
                   QMF_PROTOCAL::QmfAccHead *acc_head, 
                   QMF_PROTOCAL::QmfHead *qmf_head, 
                   wns_acc_task_struct *tsp, int force_log)
{
    comm::WnsDcObj obj;

    gettimeofday(&g_current_tv, NULL);

    if (acc_head)
    {
        obj.setUserIp(acc_head->ClientIP);
        obj.setSeq(acc_head->Seq);
        obj.setTimecost(((uint64_t)(g_current_tv.tv_sec * 1000) + (uint64_t)(g_current_tv.tv_usec / 1000)) - acc_head->Ticks);
        obj.setRspsize(acc_head->Len);
    }

    if (qmf_head)
    {
        obj.setProVersion(qmf_head->Ver);
        obj.setAppid(qmf_head->Appid);
        obj.setUin(qmf_head->Uin);
    }

    if (tsp) {
        obj.setQua(tsp->client_info_p->qua);
    }
    else {
        string qua = "";
        obj.setQua(qua);
    }

    obj.setResult(retcode, retcode);
    obj.setCgiIp(ntohl(eth1_ip));
    obj.setHostIp(ntohl(eth1_ip));												/* need local bytecode, confusing */

    if (eup) {
        obj.setServerIp(ntohl(eservice_user_get_peer_addr_network(eup)));			/* need local bytecode, confusing */
        obj.setProtocol(eservice_user_get_related_port(eup));
    }
    else if (tsp && tsp->client_info_p) {
        obj.setServerIp(tsp->client_info_p->server_ip);			/* need local bytecode, confusing */
        obj.setProtocol(tsp->client_info_p->protocol);
    }

    if (tsp && tsp->client_info_p) {
        if (!strcasecmp(tsp->client_info_p->cmd.c_str(), "wns.pushrsp")) {
            g_dbg("push rsp, ignore clog");
            return;
        }

        obj.setServiceCmd( tsp->client_info_p->cmd);
        obj.setSeq( tsp->client_info_p->seq);
        obj.setReqsize( tsp->client_info_p->req_size);
        obj.setUserIp( tsp->client_info_p->user_ip);
        obj.setHostIp( tsp->client_info_p->host_ip);
        obj.setProVersion(tsp->client_info_p->pro_version);
        obj.setAppid(tsp->client_info_p->appid);
        obj.setUin(tsp->client_info_p->uin);

        //if (force_log > 0 || tsp->client_info_p->qua.size() >= 2) {									/* only clog things HAVE qua */
        //	comm::CDcHelper::Instance()->saveDownstream(obj);
        //}

        if (g_wns_acc_main_config.clean_clog)
        {
            if (retcode != WNS_CODE_SUCC
                || (acc_head && (acc_head->Flag & 0x10)))
            {
                obj.SetClogFlag();
                g_dbg("SetClogFlag");
            }
        }
        else
        {
            obj.SetClogFlag();
            g_dbg("SetClogFlag");
        }

        if (force_log > 0 || tsp->client_info_p->qua.size() >= 2)
        { /* only clog things HAVE qua */
            comm::CDcHelper::Instance()->saveDownstream(obj);
        }
    }

    return;
}

static void 
wns_acc_logSession_push(ENUM_WNS_ERRCODE retcode, eservice_unit_t *eup,
                        QMF_PROTOCAL::QmfAccHead *acc_head, 
                        QMF_PROTOCAL::QmfHead *qmf_head)
{
    wns_acc_connect_user_context_t *user_context = NULL;

    gettimeofday(&g_current_tv, NULL);

    comm::WnsDcObj obj;
    if (acc_head)
    {
        obj.setUserIp(acc_head->ClientIP);
        obj.setSeq(acc_head->Seq);
        obj.setTimecost(0);					/* push clog cost 0 */
        obj.setRspsize(acc_head->Len);
    }

    if (qmf_head)
    {
        obj.setProVersion(qmf_head->Ver);
        obj.setAppid(qmf_head->Appid);
        obj.setUin(qmf_head->Uin);
    }

    user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
    if (user_context && user_context->last_qua.size() >= 2) {
        obj.setQua(user_context->last_qua);
    }
    else {
        string qua = "";
        obj.setQua(qua);
    }

    obj.setHostIp(ntohl(eth1_ip));												/* need local bytecode, confusing */
    obj.setServerIp(ntohl(eservice_user_get_peer_addr_network(eup)));			/* need local bytecode, confusing */
    obj.setProtocol(eservice_user_get_related_port(eup));

    std::string cmd("push.provide");
    obj.setServiceCmd(cmd);

    obj.setResult(retcode, 0);

    // push.provide
    obj.SetClogFlag();

    if (user_context && user_context->last_qua.size() >= 2) {									/* only clog things HAVE qua */
        comm::CDcHelper::Instance()->saveDownstream(obj);    
    }

    return;
}

static void 
wns_acc_LogDownstreamInfo(QMF_PROTOCAL::QmfHead &qmfhead, QMF_PROTOCAL::QmfAccHead &acchead)
{
    return;
}

static int extra_indenpent( QMF_PROTOCAL::QmfUpstream &up, QMF_PROTOCAL::QmfHead &qmfhead, int &need_replace)
{
	wup::UniAttribute<> una;
	if (up.Extra.size() > 0)
	{
		if (up.BusiControl.noexit == false)
		{
			una.decode(up.Extra);
			if(!una.containsKey(QMF_PROTOCAL::KEY_EXTRA_BUSICOMPCONTROL))
			{
				una.put<QMF_PROTOCAL::QmfBusiControl>(QMF_PROTOCAL::KEY_EXTRA_BUSICOMPCONTROL, up.BusiControl);
				una.encode(up.Extra);
				need_replace = 1;
				g_dbg("has other keys, need add extra key");
			}
		}
	}
	else
	{
		if(up.BusiControl.noexit == false)
		{
			una.put<QMF_PROTOCAL::QmfBusiControl>(QMF_PROTOCAL::KEY_EXTRA_BUSICOMPCONTROL, up.BusiControl);
			una.encode(up.Extra);
			need_replace = 1;
			g_dbg("need add extra key");
		}
	}
		
	return need_replace;
}


static int servicemap_process(QMF_PROTOCAL::QmfUpstream &up, int &need_replace)
{
	string servicecmd_key = "ServiceMap." + up.ServiceCmd;
	map<string, string>::iterator iter = g_wns_cookie_map.find(servicecmd_key);
	g_dbg("replace before servicecmd :%s" , up.ServiceCmd.c_str());
	if(iter != g_wns_cookie_map.end()){
		up.ServiceCmd = iter->second;
		need_replace = 1;
		g_dbg("replace after servicecmd :%s" , up.ServiceCmd.c_str());
	}
	return need_replace;
}

static int session_prcoess(eservice_unit_t *eup, QMF_PROTOCAL::QmfUpstream &up, QMF_PROTOCAL::QmfHead &qmfhead ,wns_acc_task_struct *tsp, int &need_repack)
{
	wns_acc_connect_user_context_t *user_context = NULL;
	if (up.sessionID != 0) {
		g_dbg("session hash: %u", up.sessionID);

		user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
		if (user_context) 
		{
			g_dbg("user_context hashid=%u qua=%s",
				user_context->session_hash, user_context->qua.c_str());
			if (user_context->session_hash != up.sessionID) 
			{
				if (up.Qua.size() >= 2 && up.DeviceInfo.size() >= 2 && up.Token.Key.size() >= 2) 
				{
					g_dbg("update session hash, old: %u", user_context->session_hash);
					user_context->session_hash = up.sessionID;
					user_context->qua = up.Qua;
					user_context->device_info = up.DeviceInfo;
					user_context->A2 = up.Token.Key;
				}
				else
				{
					g_warn("session not found, uin=%lu, cmd=%s, qua=%s",
						up.Uin, 
						up.ServiceCmd.c_str(),
						user_context->last_qua.c_str());
					tsp->error_code = WNS_CODE_ACC_INVALID_SESSIONHASH;
					return -1;
				}
			}
			else 
			{
				g_dbg("dup session hash");

				if (user_context->qua.size() < 2 
					|| user_context->device_info.size() < 2 
					|| user_context->A2.size() < 2) 
				{
					g_warn("session not found, uin=%lu, cmd=%s, qua=%s",
						up.Uin, 
						up.ServiceCmd.c_str(),
						user_context->last_qua.c_str());
					tsp->error_code = WNS_CODE_ACC_INVALID_SESSIONHASH;
					return -1;
				}

				need_repack = 1;  
				up.Qua = user_context->qua;
				up.DeviceInfo = user_context->device_info;
				up.Token.Key = user_context->A2;
				g_dbg("need replace for hash, qua size: %zu, deviceinfo size: %zu, A2 size: %zu", 
					up.Qua.size(), up.DeviceInfo.size(), up.Token.Key.size());
			}
			eservice_user_conn_set_user_data(eup, user_context);
		}
	}
	return 0;
}

static int 
wns_acc_LogUpstreamInfo(eservice_unit_t *eup, QMF_PROTOCAL::QmfHead &qmfhead, QMF_PROTOCAL::QmfAccHead &acchead, wns_acc_task_struct *tsp, eservice_buf_t **ebpp)
{
    struct eservice_buf_t *clear_buf = NULL;
    uint32_t rslen = 0, plainlen = 0;;	
    std::vector<char> vecCompt, vecDecry;
    int has_comp = 0;
    int comptlen = 0;
    int decrylen = 0;
    int ret = 0;
	int need_repack = 0;
    uint8_t *ebpp_start = NULL;
    size_t ebpp_len = 0;
    QMF_PROTOCAL::QmfUpstream up;
    wns_acc_connect_user_context_t *user_context = NULL;

    std::string userip = TOOL::Int2Str(acchead.ClientIP);

    plainlen = rslen = qmfhead.BodyLen;

    if(bit_test(&qmfhead.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_UPCOMPRESS)){
        has_comp = 1;
    }

    if (qmfhead.Ver > 1 && qmfhead.ComprLen > rslen) {
        g_dbg("ComprLen: %u, body len: %u", qmfhead.ComprLen, qmfhead.BodyLen);
        rslen = qmfhead.ComprLen;
    }

    if (rslen >= WNS_ACC_MAX_VALID_UNCOMP_PKG_LEN) {
        user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
        if (user_context){
            g_warn("invalid ComprLen: %u clientip:%s qua:%s", rslen, userip.c_str(), user_context->last_qua.c_str());
            g_warn("invalid ComprLen decode QmfHead[len=%d ver=%d enc=%d appid=%d uin=%lu b2len=%zd headlen=%d bodylen=%d flag=%d]",
                   qmfhead.Len, qmfhead.Ver, qmfhead.Enc, qmfhead.Appid, qmfhead.Uin, qmfhead.B2.size(), qmfhead.ComprLen, qmfhead.BodyLen, qmfhead.Flag);
        }
        g_warn("invalid ComprLen: %u clientip:%s", rslen, userip.c_str());
        wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, &acchead, &qmfhead, tsp, 1);
        tsp->result = -119;
        return -1;
    }

    rslen += WNS_ACC_SAFE_LEN;

    if (has_comp > 0) {
        vecDecry.resize(qmfhead.BodyLen + WNS_ACC_SAFE_LEN);
        decrylen = qmfhead.BodyLen + WNS_ACC_SAFE_LEN;

        *ebpp = eservice_buf_new(0);											/* for compress */
        comptlen = rslen;
    }
    else {
        *ebpp = eservice_buf_new(0);			/* for decrypt */
        decrylen = qmfhead.BodyLen + WNS_ACC_SAFE_LEN;
    }

    //解密QmfReq包体
    QMF::CQmfCryptor cryptor;
    if (has_comp > 0) {
        ret = cryptor.decrypt(qmfhead, &vecDecry[0], &decrylen);
        if (ret !=0) {
            g_warn("decrypt failed. uin=%lu, ret=%d", qmfhead.Uin, ret);
            tsp->error_code = WNS_CODE_ACC_DECRYPT_INVAID;
            return -1;
        }

        eservice_buf_get_space(*ebpp, rslen, &ebpp_start, &ebpp_len);

        //解压缩QmfReq
        QMF::CQmfCompressSvr compress;
        size_t compress_len = comptlen;
        ret = compress.UnCompress(qmfhead, (const char*)&vecDecry[0], (size_t)decrylen,
                                  (char*)(ebpp_start), &compress_len);

        if (ret !=0) {
            g_warn("UnCompress failed. uin=%lu, ret=%d", qmfhead.Uin, ret);
            tsp->error_code = WNS_CODE_ACC_DECRYPT_INVAID;
            return -1;
        }
        eservice_buf_commit_space(*ebpp, ebpp_start, compress_len);
        g_dbg("after decry/compress commit space uin=%lu len=%zd", qmfhead.Uin, compress_len);

        /* cheat */
        qmfhead.BodyLen = compress_len;

        comptlen = compress_len;
        ret = QMF_PROTOCAL::struct_QmfUpstream_unpack((uint8_t *)eservice_buf_start(*ebpp), &comptlen, &up, NULL);
    }
    else {
        eservice_buf_get_space(*ebpp, qmfhead.BodyLen + WNS_ACC_SAFE_LEN, &ebpp_start, &ebpp_len);
        ret = cryptor.decrypt(qmfhead, (char *)ebpp_start, &decrylen);
        if (ret !=0) {
            g_warn("decrypt failed. uin=%lu, ret=%d", qmfhead.Uin, ret);
            tsp->error_code = WNS_CODE_ACC_DECRYPT_INVAID;
            return -1;
        }
        eservice_buf_commit_space(*ebpp, ebpp_start, decrylen);
        g_dbg("after decry commit space uin=%lu len=%d", qmfhead.Uin, decrylen);

        qmfhead.BodyLen = decrylen;

        ret = QMF_PROTOCAL::struct_QmfUpstream_unpack((uint8_t *)eservice_buf_start(*ebpp), &decrylen, &up, NULL);
    }


    if (ret != 0) {
        g_warn("unpack upstream failed. uin=%lu, ret=%d", qmfhead.Uin, ret);
        tsp->error_code = WNS_CODE_ACC_DECRYPT_INVAID;
        return -1;
    } else {

		/*up.ServiceCmd*/
		servicemap_process(up, need_repack);
		//1 需要重新打包upstream 0 不需要
		extra_indenpent(up, qmfhead, need_repack);

        /*
           g_dbg("retry flag: %u, retry count: %u, pkg id: %lld", up.retryinfo.Flag, up.retryinfo.RetryCount, up.retryinfo.PkgId);
           if (up.retryinfo.Flag > 0)
           tsp->cmd_can_retry = 1;
           else
           tsp->cmd_can_retry = 0;

*/
		ret = session_prcoess(eup, up, qmfhead, tsp, need_repack);
		if(ret != 0)
		{
			g_warn("session_prcoess failed. uin=%lu, ret=%d", qmfhead.Uin, ret);
			return ret;
		}

		user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
		if (user_context && need_repack) 
		{
			
			eservice_buf_drain(*ebpp, eservice_buf_datalen(*ebpp));
			eservice_buf_get_space(*ebpp, qmfhead.BodyLen + WNS_ACC_SAFE_LEN, &ebpp_start, &ebpp_len);
			/* pack new upstream */
			int32_t unpack_len = ebpp_len;
			ret =  QMF_PROTOCAL::struct_QmfUpstream_pack(&up, (uint8_t *)ebpp_start, &unpack_len, NULL);
			if (ret != 0) {
				g_warn("struct_QmfUpstream_pack err. ret=%d\n", ret);
				return -1;
			}
			eservice_buf_commit_space(*ebpp, ebpp_start, unpack_len);

			qmfhead.BodyLen = ebpp_len;
		}
		

        /* reserve data */
        if (tsp && tsp->client_info_p) {
            /* reserve qua */
            if (up.Qua.size() >= 2) {				/* only clog things HAVE qua */			
                tsp->client_info_p->qua = up.Qua;
            }

            tsp->client_info_p->cmd = up.ServiceCmd;
            tsp->client_info_p->seq = up.Seq;
            tsp->client_info_p->req_size = plainlen;
            tsp->client_info_p->user_ip = ntohl(acchead.ClientIP);
            tsp->client_info_p->pro_version = qmfhead.Ver;
            tsp->client_info_p->appid = up.Appid;   //qmfhead.Appid;
            tsp->client_info_p->uin = qmfhead.Uin;
            tsp->client_info_p->host_ip = ntohl(eth1_ip);

            if (!strcasecmp(tsp->client_info_p->cmd.c_str(), "wns.pushrsp")) {
                g_dbg("push rsp need not response");
                tsp->need_not_rsp = 1;
            }
        }		
    }

    return 0;
}


static int
wns_acc_response_push_client_invalid(eservice_unit_t *eup, wns_acc_task_struct *tsp, QMF_PROTOCAL::QmfAccHead *accheadp, int acchead_len)
{
    struct eservice_buf_t *send_ebp = NULL;
    int res = -255;
    char buf[1024];
    char buf2[1024];
    uint32_t buflen = sizeof(buf);
    QMF_PROTOCAL::QmfHeadParser parser;
    QMF_PROTOCAL::QmfHead qmfhead;

    send_ebp = eservice_buf_new(0);
    if (!send_ebp) {
        g_warn("eservice_buf_new() failed");
        return -119;
    }

    g_dbg("=> wns_acc_response_push_client_invalid(), eup: %p, tsp: %p", eup, tsp);

    /* dummy */
    parser.EncodeQmfHead(buf, (int *)&buflen, qmfhead, buf2, 0);

    accheadp->Flag &= ~WNS_FLAG_CLIENT_EXISTS_BIT;
    accheadp->Len = acchead_len + sizeof(qmfhead);
    parser.EncodeQmfAccHead(*accheadp);

    if (eservice_buf_add(send_ebp,  accheadp, acchead_len) < 0 || eservice_buf_add(send_ebp, &qmfhead, sizeof(qmfhead)) < 0) {
        g_warn("eservice_buf_add() failed: %s", strerror(errno));
        ESERVICE_DESTROY_EBP(send_ebp);
        return -1;
    }

    res = eservice_user_push_outbuf(eup, send_ebp);
    if (res < 0) {
        g_warn("eservice_user_push_outbuf() failed: %d", res);
        return res;
    }

    return 0;
}

static int
wns_acc_response_push_client_ok(eservice_unit_t *eup, wns_acc_task_struct *tsp, QMF_PROTOCAL::QmfAccHead *accheadp, int acchead_len)
{
    struct eservice_buf_t *send_ebp = NULL;
    int res = -255;
    char buf[1024];
    char buf2[1024];
    uint32_t buflen = sizeof(buf);
    QMF_PROTOCAL::QmfHeadParser parser;
    QMF_PROTOCAL::QmfHead qmfhead;

    send_ebp = eservice_buf_new(0);
    if (!send_ebp) {
        g_warn("eservice_buf_new() failed");
        return -119;
    }

    g_dbg("=> wns_acc_response_push_client_ok(), eup: %p, tsp: %p", eup, tsp);

    /* dummy */
    parser.EncodeQmfHead(buf, (int *)&buflen, qmfhead, buf2, 0);

    accheadp->Flag |= WNS_FLAG_CLIENT_EXISTS_BIT;
    accheadp->Len = acchead_len + sizeof(qmfhead);
    parser.EncodeQmfAccHead(*accheadp);

    if (eservice_buf_add(send_ebp,  accheadp, acchead_len) < 0 || eservice_buf_add(send_ebp, &qmfhead, sizeof(qmfhead)) < 0) {
        g_warn("eservice_buf_add() failed: %s", strerror(errno));
        ESERVICE_DESTROY_EBP(send_ebp);
        return -1;
    }

    res = eservice_user_push_outbuf(eup, send_ebp);
    if (res < 0) {
        g_warn("eservice_user_push_outbuf() failed: %d", res);
        return res;
    }

    return 0;
}


static int
wns_acc_get_proper_async(eservice_unit_t *eup, struct eservice_buf_t *pkg_data, ssize_t pkg_len, struct eservice_unit_t **eupp, wns_acc_task_struct **tspp)
{
    int res = -255;
    QMF_PROTOCAL::QmfAccHead _acc_head;
    QMF_PROTOCAL::QmfHeadParser m_protocal_parser;
    uint8_t *data_start = NULL;
    int _acc_head_len = 0;
    QMF_PROTOCAL::QmfHead _qmf_head;

    if (!eup || !pkg_data || !eupp || !tspp)
        return -1;

    *tspp = NULL;

    pkg_len = eservice_buf_datalen(pkg_data);
    eservice_buf_make_linearizes(pkg_data, pkg_len);
    data_start = eservice_buf_start(pkg_data);

    /*************Deco AccHead **********/
    res = m_protocal_parser.DecodeQmfAccHead((const char *)data_start, pkg_len, _acc_head);
    if (res != 0) {
        wns_acc_logSession(WNS_CODE_ACC_SERVER_ACCUNPACK, eup, NULL, NULL, NULL);
        g_warn("Decode QmfAccHead ERR. ret=%d", res);
        return -1;
    }

    g_dbg("Data Come from Dispatcher(%s:%d), clientid=%lu, len=%zd", 
          eservice_user_get_peer_addr_str(eup), eservice_user_get_peer_addr_port(eup), 
          _acc_head.ClientID, pkg_len);

    g_msg("Decode QmfAcc[len=%d ver=%d ip=%u port=%d cliendid=%lu seq=%d accflag:%d]",
          _acc_head.Len, _acc_head.Ver, _acc_head.ClientIP, _acc_head.ClientPort, _acc_head.ClientID,
          _acc_head.Seq, _acc_head.Flag);

    /* clear async map if exists */
    std::map<uint64_t, void *>::iterator the_iter = wns_async_map.find(_acc_head.ClientID);
    if (the_iter != wns_async_map.end() && the_iter->second != NULL) {
        *tspp = (wns_acc_task_struct *)(the_iter->second);
        *eupp = (*tspp)->eup; //(struct eservice_unit_t *)(get_eup(_acc_head.ClientID));

        g_dbg("async tsp load, %lu -> %p, eup: %p", 
              the_iter->first, the_iter->second, eupp);

        if(*tspp && ((*tspp)->client_info_p)) //设置
        {
            wns_acc_update_success_L5((*tspp)->client_info_p->qos, 1000*(get_tick_count() - _acc_head.Ticks));
        }

        wns_async_map.erase(the_iter);
        wns_acc_del_waiting_task_struct(*tspp);
    }
    else {
        g_warn("invalid async, not found: %lu", _acc_head.ClientID);
        wns_acc_logSession(WNS_CODE_ACC_CLIENT_INVAID, eup, &_acc_head, &_qmf_head, NULL);
        return -3;
    }

    /*************Deco QmfHead************/
    _acc_head_len = m_protocal_parser.GetAccHeadLen(_acc_head);

    res = m_protocal_parser.DecodeQmfHead((const char *)data_start + _acc_head_len, pkg_len -_acc_head_len, _qmf_head);
    if (res != 0) {
        g_warn("Decode QmfHead ERR. ret=%d", res);
        wns_acc_logSession(WNS_CODE_ACC_SERVER_QMFUNPACK, eup, &_acc_head, NULL, *tspp);
        return -2;
    }

    // NOTE: 这里更新打包后qmfhead !!!
    // 标识acc支持sessionid
    // wp8不支持...
    wns_acc_connect_user_context_t *user_context;
    user_context = 
        (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data((*tspp)->eup);
    if (!user_context || user_context->last_qua != "V1_WIN_QZ_1.0.0_001_WRGW_A")
    {
        // cdn加速来的,不支持session
        if ((*tspp)->client_port == 0)
        {
            _qmf_head.Flag |= (1 << (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_SESSION); 
        }
		_qmf_head.Flag |=  (1 << (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_SHORTCMD);
        uint32_t *flag = (uint32_t *)(data_start + _acc_head_len + (4 + 4 + 1 + 1));
        *flag = htonl(_qmf_head.Flag);


        // disable when overload
        if (g_total_connection_num > (g_max_connection_num  * 85 / 100))
        { 
            _qmf_head.Flag |= (1 << (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_OVERLOAD);
        }
    }
    else
    {
        if (user_context) g_dbg("disable session for %s", user_context->last_qua.c_str());
    }

    // 统计相关

    wns_30s.dispatch_rsp_num++;

    eservice_buf_drain(pkg_data, _acc_head_len);
    ss_list_push_head_struct(&((*tspp)->output_ebp_list), eservice_buf_buf2list(pkg_data));

    g_msg("Ready send 2 client. sendlen=%zd", eservice_buf_datalen(pkg_data));

    wns_30s.rsp_num++;
    wns_acc_logSession(WNS_CODE_SUCC, eup, &_acc_head, &_qmf_head, *tspp);

    return 0;
}

static int
wns_acc_get_push_client(eservice_unit_t *eup, struct eservice_unit_t **eupp, wns_acc_task_struct *tsp)
{
    int res = -255;
    QMF_PROTOCAL::QmfAccHead _acc_head;
    QMF_PROTOCAL::QmfHead _qmf_head;
    QMF_PROTOCAL::QmfHeadParser m_protocal_parser;
    wns_acc_connect_user_context_t *user_context = NULL;
    uint8_t *data_start = NULL;
    int _acc_head_len = 0;
    uint32_t pkg_len = 0;
    int idle_second = 0, use_second = 0;

    if (!eup || !tsp || !(tsp->input_ebp) || !eupp)
        return -1;

    pkg_len = eservice_buf_datalen(tsp->input_ebp);
    eservice_buf_make_linearizes(tsp->input_ebp, pkg_len);
    data_start = eservice_buf_start(tsp->input_ebp);

    /*************Deco AccHead **********/
    res = m_protocal_parser.DecodeQmfAccHead((const char *)data_start, pkg_len, _acc_head);
    if (res != 0) {
        g_warn("Decode QmfAccHead ERR. ret=%d", res);
        return -1;
    }

    user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
    if (user_context) {
        user_context->datacomed = 1;
        eservice_user_conn_set_user_data(eup, user_context);
    }

    g_dbg("Data Come from Pushserver(%s:%d), clientid=%lu, len=%d", 
          eservice_user_get_peer_addr_str(eup), eservice_user_get_peer_addr_port(eup), _acc_head.ClientID, pkg_len);

    g_msg("Decode QmfAcc[len=%d ver=%d ip=%u port=%d cliendid=%lu]",
          _acc_head.Len, _acc_head.Ver, _acc_head.ClientIP, _acc_head.ClientPort, _acc_head.ClientID);

    _acc_head_len = m_protocal_parser.GetAccHeadLen(_acc_head);
    eservice_buf_drain(tsp->input_ebp, _acc_head_len);			/* remove acc head */

    int idx = get_eup(_acc_head.ClientID);
    *eupp = eservice_user_eup_from_index(idx);
    if (eservice_user_eup_is_invalid(*eupp)) {
        g_warn("client connection invalid, eup: %p", *eupp);
        wns_acc_response_push_client_invalid(eup, tsp, &_acc_head, _acc_head_len);
        return -2;
    }

    wns_acc_response_push_client_ok(eup, tsp, &_acc_head, _acc_head_len);

    /* statistic */
    push_statistic.push_num++;
    idle_second = eservice_user_conn_idle_seconds(*eupp);
    if (idle_second < 30) {
        push_statistic.less_30++;
    }
    else if (idle_second < 60) {
        push_statistic.s_30_60++;
    }
    else if (idle_second < 180) {
        push_statistic.s_60_180++;
    }
    else if (idle_second < 300) {
        push_statistic.s_180_300++;
    }
    else if (idle_second < 600) {
        push_statistic.s_300_600++;
    }
    else if (idle_second < 900) {
        push_statistic.s_600_900++;
    }
    else if (idle_second < 1200) {
        push_statistic.s_900_1200++;
    }
    else
        push_statistic.s_1200_1500++;

    use_second = eservice_user_conn_use_seconds(*eupp);
    if (use_second < 30) {
        push_statistic.init_less_30++;
    }
    else if (use_second < 60) {
        push_statistic.init_s_30_60++;
    }
    else if (use_second < 180) {
        push_statistic.init_s_60_180++;
    }
    else if (use_second < 300) {
        push_statistic.init_s_180_300++;
    }
    else if (use_second < 600) {
        push_statistic.init_s_300_600++;
    }
    else if (use_second < 900) {
        push_statistic.init_s_600_900++;
    }
    else if (use_second < 1200) {
        push_statistic.init_s_900_1200++;
    }
    else
        push_statistic.init_s_1200_1500++;

    /* decode qmf header for clog */
    res = m_protocal_parser.DecodeQmfHead((const char *)data_start + _acc_head_len, pkg_len -_acc_head_len, _qmf_head);
    if (res != 0) {
        g_warn("Decode QmfHead ERR. ret=%d", res);
        return -2;
    }

    wns_acc_logSession_push(WNS_CODE_SUCC, *eupp, &_acc_head, &_qmf_head);
    wns_30s.push_num++;

    ss_list_push_head_struct(&(tsp->output_ebp_list), eservice_buf_buf2list(tsp->input_ebp));	
    g_msg("Ready send 2 client. sendlen=%zd", eservice_buf_datalen(tsp->input_ebp));
    tsp->input_ebp = NULL;									/* already moved to out list */

    return 0;
}

static int
wns_acc_get_L5(int appid)
{
    int res = -255;
    std::string errmsg;

    if(appid < 0)
    {
        return -1;
    }

    if (appid == 65538) //空间的L5
    {
        qos_dispatch._modid = g_wns_acc_main_config.dest_modid;
        qos_dispatch._cmd = g_wns_acc_main_config.dest_cmd;
    }
    else //接入wns的其他业务
    {
        qos_dispatch._modid = g_wns_acc_main_config.other_dest_modid;
        qos_dispatch._cmd = g_wns_acc_main_config.other_dest_cmd;
    }

    res = ApiGetRoute(qos_dispatch, 0.01, errmsg);
    if (res >= 0) {
        snprintf(qos_dispatch_port_str, sizeof(qos_dispatch_port_str) - 1, "%u", qos_dispatch._host_port);
    }
    else {
        g_warn("mod=%d cmd=%d ApiGetRoute failed ret=%d msg=%s",
               qos_dispatch._modid, qos_dispatch._cmd, res, errmsg.c_str());

        return -1;
    }

    return 0;
}

static int wns_acc_update_failed_L5(QOSREQUEST &qos,int usetime_usec)
{
    std::string errmsg;
    g_warn("update failed L5, mod id: %d, cmd id: %d, ip: %s, port: %u", qos._modid, qos._cmd, qos._host_ip.c_str(), qos._host_port);

    if(usetime_usec >= 0)
        ApiRouteResultUpdate(qos, -1, usetime_usec, errmsg);
    else
        ApiRouteResultUpdate(qos, -1, 100, errmsg);
    return 0;
}

static int wns_acc_update_success_L5(QOSREQUEST &qos,int usetime_usec)
{
    std::string errmsg;

    g_dbg("update successful L5, mod id: %d, cmd id: %d, ip: %s, port: %u", qos._modid, qos._cmd, qos._host_ip.c_str(), qos._host_port);
    if(usetime_usec > 0)
        ApiRouteResultUpdate(qos, 0, usetime_usec, errmsg);
    else
        ApiRouteResultUpdate(qos, 0, 1, errmsg);

    return 0;
}

static int
wns_make_empty_response(wns_acc_task_struct *tsp, int wns_code)
{
    struct dbl_list_head *ptr = NULL, *n = NULL;
    struct eservice_buf_t *ebp = NULL;
    QMF_PROTOCAL::QmfHead _rep_qmf_head;
    QMF_PROTOCAL::QmfDownstream qmfrsp;		/* dummy */
    int qmfbufflen, qmfbodylen;
    uint8_t busi_buf[MAX_LINE_LEN], pkg_buf[2 * MAX_LINE_LEN];
    int res = -255;
    QMF_PROTOCAL::QmfHeadParser m_protocal_parser;

    g_dbg("=> wns_make_empty_response(), tsp: %p, wns_code: %d", tsp, wns_code);

    if (!tsp)
        return 0;

    if (!(tsp->eup)) {
        g_warn("no any client eup, tsp: %p", tsp);
        tsp->result = -119;
        return -1;
    }

    qmfbufflen = sizeof(pkg_buf);

    qmfbodylen = sizeof (busi_buf);

    qmfrsp.WnsCode = wns_code;
    qmfrsp.BizCode = 0;
    qmfrsp.Seq = tsp->client_info_p->seq;
    res = QMF_PROTOCAL::struct_QmfDownstream_pack(&qmfrsp, busi_buf, &qmfbodylen, NULL);
    if (res) {
        g_warn("struct_QmfDownstream_pack err. res=%d", res);
        tsp->result = -119;
        return -1;
    }

    _rep_qmf_head.init();
    _rep_qmf_head.Ver = (uint8_t)(tsp->client_info_p->pro_version);
    _rep_qmf_head.Uin = tsp->client_info_p->uin;
    _rep_qmf_head.Appid = tsp->client_info_p->appid;
    _rep_qmf_head.seq = tsp->client_info_p->seq;
    m_protocal_parser.EncodeQmfHead((char *)pkg_buf, &qmfbufflen, _rep_qmf_head, (char *)busi_buf, qmfbodylen);

    if (qmfbufflen >= (int)sizeof(pkg_buf)) {
        g_warn("invalid qmfbufflen: %u", qmfbufflen);
        tsp->result = -119;
        return -1;
    }

    ebp = eservice_buf_new(qmfbufflen);		
    eservice_buf_add(ebp, pkg_buf, qmfbufflen);

    if (tsp->protocol_type == 2) {
        char http_length_buffer[MAX_LINE_LEN];
        int snlen = snprintf(http_length_buffer, sizeof(http_length_buffer) - 1, 
                             "%zd\r\n\r\n", eservice_buf_datalen(ebp));
        eservice_buf_prepend(ebp, http_length_buffer, snlen);
        eservice_buf_prepend(ebp, http_fake_header, http_fake_header_len); 
    }
    if (tsp->client_port) prepend_proxy_header(tsp, ebp);

    /* send to client */
    eservice_buf_set_control(ebp, 1, pkg_send_failed_cb, NULL);

    res = eservice_user_push_outbuf(tsp->eup, ebp);
    if (res < 0) {
        g_warn("eservice_user_push_outbuf() failed: %d", res);
    }

    return 0;
}

#ifndef	USE_IMME_TCP
static eservice_callback_return_val 
dispatch_long_conn_data_arrive_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
    struct eservice_buf_t *pkg_data = NULL;
    ssize_t pkg_len = 0, buf_data_len = 0;
    int res = -255;
    wns_acc_task_struct *the_tsp = NULL;
    wns_inner_psy_header_t *psy_headp = NULL;
    struct eservice_buf_t *the_ebp = NULL;
    struct eservice_unit_t *the_eup = NULL;
    char http_length_buffer[MAX_LINE_LEN];
    int snlen = 0;
    struct dbl_list_head *dptr = NULL;
    std::string errmsg;

    g_dbg("=> dispatch_long_conn_data_arrive_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    while (1) {
        pkg_data = NULL;
        pkg_len = 0;
        psy_headp = NULL;

        buf_data_len = eservice_user_datalen(eup);
        if (buf_data_len < 0) {
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
            return eservice_cb_data_arrive_fatal;
        }

        if (buf_data_len < (ssize_t)sizeof(wns_inner_psy_header_t) || buf_data_len > WNS_ACC_MAX_VALID_PKG_LEN) {
            return eservice_cb_data_arrive_not_enough;
        }

        /* parse protocol */
        psy_headp = (wns_inner_psy_header_t *)(eservice_user_pos_ex(eup, 0, (ssize_t)(sizeof(wns_inner_psy_header_t))));
        if (psy_headp == NULL)
        {
            g_warn("eservice_user_pos_ex return NULL");
            return eservice_cb_data_arrive_fatal;
        }

        if (psy_headp->SOH != STX_INNER_SIGN) {
            g_warn("unknown protocol: %#0x", psy_headp->SOH);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
            return eservice_cb_data_arrive_fatal;
        }

        pkg_len = ntohl(psy_headp->Len);
        g_msg("wns inner protocol, len: %zd", pkg_len);

        if (pkg_len <= (ssize_t)sizeof(wns_inner_psy_header_t) || pkg_len > WNS_ACC_MAX_VALID_PKG_LEN) {
            g_warn("invalid wns inner pkg len: %zd", pkg_len);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
            return eservice_cb_data_arrive_fatal;
        }

        if (buf_data_len < pkg_len) {
            eservice_user_next_need(eup, pkg_len - buf_data_len);
            return eservice_cb_data_arrive_not_enough;
        }

        res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
        if (res < 0 || !pkg_data) {
            g_warn("eservice_user_pop_inbuf() failed: %d", res);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
            return eservice_cb_data_arrive_fatal;
        }

        res = wns_acc_get_proper_async(eup, pkg_data, pkg_len, &the_eup, &the_tsp);
        if (res < 0) {
            g_msg("invalid async");
            eservice_buf_free(pkg_data);
            continue;
        }

        if (the_tsp && the_tsp->output_ebp_list.number > 0) {
            dptr = ss_list_pop_head_struct(&(the_tsp->output_ebp_list));
            if (dptr) {
                the_ebp = eservice_buf_list2buf(dptr);
                eservice_buf_set_control(the_ebp, 3, pkg_send_failed_cb, NULL);

                if (the_tsp->protocol_type == 2) {					/* add HTTP fake header */			
                    snlen = snprintf(http_length_buffer, sizeof(http_length_buffer) - 1, 
                                     "%zd\r\n\r\n", eservice_buf_datalen(the_ebp));
                    eservice_buf_prepend(the_ebp, http_length_buffer, snlen);
                    eservice_buf_prepend(the_ebp, http_fake_header, http_fake_header_len);
                }

                // 如果是oc点加速过来的,需要加上相应的头
                prepend_proxy_header(the_tsp, the_ebp);

                res = eservice_user_push_outbuf(the_eup, the_ebp);
                if (res < 0) {
                    g_msg("eservice_user_push_outbuf() failed: %d", res);

                    wns_acc_logSession(WNS_CODE_ACC_CLIENT_SND_ERR, eup, NULL, NULL, the_tsp);
                }
            }
        }

        wns_acc_free_task_struct(the_tsp);
        the_tsp = NULL;
        dptr = NULL;
    }	

    return eservice_cb_data_arrive_ok;
}

static eservice_callback_return_val 
dispatch_long_conn_terminate_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
    g_warn("=> dispatch_long_conn_terminate_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    return eservice_cb_success;
}

static eservice_callback_return_val 
dispatch_long_conn_timeout_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
    //g_dbg("=> dispatch_long_conn_timeout_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);
    return eservice_cb_success;
}

static struct eservice_unit_t *
wns_acc_find_proper_dispatch(wns_acc_task_struct *tsp)
{
    struct timeval to = {1, 0};
    acc_l5conn_key_t key;
    struct eservice_unit_t *dispatch_long_eup = NULL;
    std::map<uint64_t, struct eservice_unit_t *>::iterator it;

    int app_type = -1;

    if (tsp && tsp->client_info_p)
    {
        if (wns_acc_get_L5(tsp->client_info_p->appid) < 0)
            return NULL;
    }
    else
    {
        g_warn("find disp long con appid fail");
        return NULL;
    }

    /* update tsp server info */
    if (tsp && tsp->client_info_p) {
        tsp->client_info_p->server_ip = ntohl((uint32_t)(inet_addr(qos_dispatch._host_ip.c_str())));
        if (tsp->client_info_p->appid > 0)
        {
            if (tsp->client_info_p->appid == 65538) //QZONE
            {
                app_type = 0;
                g_dbg("Qzone");
            }
            else //其他接入WNS的应用
            {
                app_type = 1;
                g_dbg("Other app");
            }
        }
    }

    if(app_type < 0)
    {
        if (tsp && tsp->client_info_p)
        {
           g_warn("get disp long con apptype fail. appid:%d",tsp->client_info_p->appid);
        }

      // return NULL;

        app_type = 1; //找不到合理的appid 归类到非QZONE 应用

    }



    key.ip_port.ip = inet_addr(qos_dispatch._host_ip.c_str());
    key.ip_port.port = (uint32_t)(qos_dispatch._host_port);

    it = dispatch_long_eup_map[app_type].find(key.key64);
    if (it == dispatch_long_eup_map[app_type].end()) {
        g_dbg("new conn for %s:%u, key: %lu,app_type:%d",
              qos_dispatch._host_ip.c_str(), qos_dispatch._host_port, key.key64,app_type);
        dispatch_long_eup = NULL;
    }
    else {
        g_dbg("reuse conn for %s:%u, key: %lu,app type:%d",
              qos_dispatch._host_ip.c_str(), qos_dispatch._host_port, key.key64,app_type);
        dispatch_long_eup = it->second;
    }

    if (eservice_user_eup_is_invalid(dispatch_long_eup)) {
        g_warn("dispatch connection invalid, eup: %p", dispatch_long_eup);
        dispatch_long_eup = NULL;
    }

    if (!dispatch_long_eup) {
        dispatch_long_eup = eservice_user_register_tcp_conn(NULL, qos_dispatch._host_ip.c_str(), qos_dispatch_port_str, 1000, &to);
        if (!dispatch_long_eup) {
            g_warn("eservice_user_register_tcp_conn() failed");
            return NULL;
        }

        g_dbg("new dispatch connection: %p app type:%d", dispatch_long_eup,app_type);

        eservice_user_set_tcp_rw_buffer_size(dispatch_long_eup, 2 * 1024 * 1024, 2 * 1024 * 1024);		/* 2M */
        eservice_user_set_tcp_conn_data_arrive_cb(dispatch_long_eup, dispatch_long_conn_data_arrive_cb, (void *)dispatch_long_eup);
        eservice_user_set_tcp_conn_terminate_cb(dispatch_long_eup, dispatch_long_conn_terminate_cb, (void *)dispatch_long_eup);
        eservice_user_set_tcp_conn_timeout_cb(dispatch_long_eup, dispatch_long_conn_timeout_cb, (void *)dispatch_long_eup, 3000);		/* 3s timeout */

        dispatch_long_eup_map[app_type][key.key64] = dispatch_long_eup;
    }
    else {
        g_dbg("reuse dispatch connection: %p app type:%d", dispatch_long_eup,app_type);
    }

    return dispatch_long_eup;
}


#else
static eservice_callback_return_val 
dispatch_conn_data_arrive_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
    struct eservice_buf_t *pkg_data = NULL;
    ssize_t pkg_len = 0, buf_data_len = 0;
    int res = -255;
    wns_acc_task_struct *the_tsp = NULL;
    wns_inner_psy_header_t *psy_headp = NULL;
    struct eservice_buf_t *the_ebp = NULL;
    struct eservice_unit_t *the_eup = NULL;
    char http_length_buffer[MAX_LINE_LEN];
    int snlen = 0;
    struct dbl_list_head *dptr = NULL;
    std::string errmsg;

    g_dbg("=> dispatch_conn_data_arrive_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    while (1) {
        pkg_data = NULL;
        pkg_len = 0;

        buf_data_len = eservice_user_datalen(eup);
        if (buf_data_len < 0) {
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL);
            return eservice_cb_data_arrive_fatal;
        }

        if (buf_data_len < sizeof(wns_inner_psy_header_t) || buf_data_len > WNS_ACC_MAX_VALID_PKG_LEN) {
            return eservice_cb_data_arrive_not_enough;
        }

        /* parse protocol */
        psy_headp = (wns_inner_psy_header_t *)(eservice_user_pos_ex(eup, 0, (ssize_t)(sizeof(wns_inner_psy_header_t))));
        if (psy_headp == NULL)
        {
            g_warn("eservice_user_pos_ex return NULL");
            return eservice_cb_data_arrive_fatal;
        }
        if (psy_headp->SOH != WNS_INNER_SIGN) {
            g_warn("unknown protocol: %#0x", psy_headp->SOH);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL);
            return eservice_cb_data_arrive_fatal;
        }

        pkg_len = ntohl(psy_headp->Len);
        g_msg("wns inner protocol, len: %u", pkg_len);

        if (pkg_len <= sizeof(wns_inner_psy_header_t) || pkg_len > WNS_ACC_MAX_VALID_PKG_LEN) {
            g_warn("invalid wns inner pkg len: %u", pkg_len);
			wns_acc_logSession(WNS_CODE_ACC_BACKSERVER_ERROR_DATA, eup, NULL, NULL, NULL);
            return eservice_cb_data_arrive_fatal;
        }

        if (buf_data_len < pkg_len) {
            eservice_user_next_need(eup, pkg_len - buf_data_len);
            return eservice_cb_data_arrive_not_enough;
        }

        res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
        if (res < 0 || !pkg_data) {
            g_warn("eservice_user_pop_inbuf() failed: %d", res);
            return eservice_cb_data_arrive_fatal;
        }

        res = wns_acc_get_proper_async(eup, pkg_data, pkg_len, &the_eup, &the_tsp);
        if (res < 0) {
            g_msg("invalid async");
            eservice_buf_free(pkg_data);
            break;
        }

        if (the_tsp && the_tsp->output_ebp_list.number > 0) {
            dptr = ss_list_pop_head_struct(&(the_tsp->output_ebp_list));
            if (dptr) {
                the_ebp = eservice_buf_list2buf(dptr);
                eservice_buf_set_control(the_ebp, 3, pkg_send_failed_cb, NULL);

                if (the_tsp->protocol_type == 2) {					/* add HTTP fake header */			
                    snlen = snprintf(http_length_buffer, sizeof(http_length_buffer) - 1, "%d\r\n\r\n", eservice_buf_datalen(the_ebp));
                    eservice_buf_prepend(the_ebp, http_length_buffer, snlen);
                    eservice_buf_prepend(the_ebp, http_fake_header, http_fake_header_len);
                }

                res = eservice_user_push_outbuf(the_eup, the_ebp);
                if (res < 0) {
                    g_warn("eservice_user_push_outbuf() failed: %d", res);

                    wns_acc_logSession(WNS_CODE_ACC_CLIENT_SND_ERR, eup, NULL, NULL, the_tsp);
                }
            }
        }

        wns_acc_free_task_struct(the_tsp);
        the_tsp = NULL;

        break;			/* only one response */
    }	

    return eservice_cb_data_arrive_finish;
}

static eservice_callback_return_val 
dispatch_conn_terminate_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
    g_dbg("=> dispatch_conn_terminate_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    return eservice_cb_success;
}

static eservice_callback_return_val 
dispatch_conn_timeout_cb(struct eservice_manager_t *egr, struct eservice_unit_t *eup, void *earg, void *garg)
{
    g_dbg("=> dispatch_conn_timeout_cb(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    if (eservice_user_conn_is_connecting(eup)) {
        if (eservice_user_conn_idle_seconds(eup) > WNS_ACC_MAX_CONN_CONNECTING_SECONDS) {
            g_warn("dispatch connecting timeout cut, eup: %p, timeout: %d", eup, eservice_user_conn_idle_seconds(eup));

            /* report conn error */
            wns_acc_update_failed_L5(eup);

            eservice_user_destroy_tcp_conn(eup);

            return eservice_cb_success;
        }
    }

    if (eservice_user_conn_idle_seconds(eup) >= WNS_ACC_MAX_TSP_PROCESS_SECONDS) {
        g_warn("dispatch connection timeout cut, eup: %p, timeout: %d", eup, eservice_user_conn_idle_seconds(eup));
        eservice_user_destroy_tcp_conn(eup);
    }

    return eservice_cb_success;
}
#endif

static int
wns_connect_to_dispatch_and_send_data(eservice_unit_t *eup, wns_acc_task_struct *tsp, struct eservice_buf_t *send_ebp)
{
    int res = -255;
    struct timeval to = {1, 0};
    struct eservice_unit_t *dispatch_long_eup = NULL;

#ifndef	USE_IMME_TCP
    dispatch_long_eup = wns_acc_find_proper_dispatch(tsp);
    if (!dispatch_long_eup) {
        g_warn("wns_acc_find_proper_dispatch() failed");
        return -1;
    }

    if(tsp->client_info_p)
    {
        tsp->client_info_p->qos = qos_dispatch;
    }

    /* send_ebp pushed to eup, outer losts it's control */
    if (send_ebp) {
        res = eservice_user_push_outbuf(dispatch_long_eup, send_ebp);
        if (res < 0) {
            // only write failed
            if (res == eservice_user_errcode_write) wns_acc_update_failed_L5(qos_dispatch, 1);
        }
    }

#else
    wns_acc_get_L5();

    /* update tsp server info */
    if (tsp && tsp->client_info_p) {
        tsp->client_info_p->server_ip = ntohl((uint32_t)(inet_addr(qos_dispatch._host_ip.c_str())));
    }

    dispatch_eup = eservice_user_register_tcp_imme(NULL, qos_dispatch._host_ip.c_str(), qos_dispatch_port_str, 1000, &to);
    if (!dispatch_eup) {
        g_warn("eservice_user_register_tcp_imme() failed");
        return -1;
    }

    wns_acc_update_success_L5(dispatch_eup);

    eservice_user_set_tcp_conn_data_arrive_cb(dispatch_eup, dispatch_conn_data_arrive_cb, (void *)dispatch_eup);
    eservice_user_set_tcp_conn_terminate_cb(dispatch_eup, dispatch_conn_terminate_cb, (void *)dispatch_eup);
    eservice_user_set_tcp_conn_timeout_cb(dispatch_eup, dispatch_conn_timeout_cb, (void *)dispatch_eup, 1000);		/* 1s timeout */


    if (tsp->need_not_rsp == 1) {
        g_dbg("set close after write, tsp: %p", tsp);
        eservice_user_set_close(dispatch_eup);
    }

    /* send_ebp pushed to eup, outer losts it's control */
    if (send_ebp) {
        eservice_user_push_outbuf(dispatch_eup, send_ebp);
    }
#endif

    return 0;
}

static int 
wns_pkg_error_data(eservice_unit_t *eup, 
                   wns_acc_task_struct *tsp, 
                   QMF_PROTOCAL::QmfHeadParser &m_protocal_parser, 
                   QMF_PROTOCAL::QmfHead &_qmf_head,
                   const int error_code)
{
    struct dbl_list_head *ptr = NULL, *n = NULL;
    struct eservice_buf_t *ebp = NULL;
    QMF_PROTOCAL::QmfHead _rep_qmf_head;
    QMF_PROTOCAL::QmfDownstream qmfrsp;		/* dummy */
    int qmfbufflen, qmfbodylen;
    uint8_t busi_buf[MAX_LINE_LEN], pkg_buf[2 * MAX_LINE_LEN];
    int res = 0;
    /* free output */
    DBL_LIST_FOR_EACH_SAFE(ptr, n, &(tsp->output_ebp_list.head)) {
        if (!ptr)
            break;

        ebp = eservice_buf_list2buf(ptr);
        ESERVICE_DESTROY_EBP(ebp);
    }
    ss_list_init_head_struct(&(tsp->output_ebp_list));

    qmfbufflen = sizeof(pkg_buf);

    qmfbodylen = sizeof (busi_buf);

    qmfrsp.WnsCode = error_code;
    qmfrsp.BizCode = 0;
    qmfrsp.Seq = _qmf_head.seq;
    res = QMF_PROTOCAL::struct_QmfDownstream_pack(&qmfrsp, busi_buf, &qmfbodylen, NULL);
    if (res) {
        g_warn("struct_QmfDownstream_pack err. res=%d", res);
        tsp->result = -119;
        return -1;
    }
    _rep_qmf_head.init();
    _rep_qmf_head.Ver = _qmf_head.Ver;
    _rep_qmf_head.Uin = _qmf_head.Uin;
    _rep_qmf_head.Appid = _qmf_head.Appid;
    _rep_qmf_head.seq = _qmf_head.seq;
    m_protocal_parser.EncodeQmfHead((char *)pkg_buf, &qmfbufflen, _rep_qmf_head, (char *)busi_buf, qmfbodylen);

    if (qmfbufflen >= (int)sizeof(pkg_buf)) {
        g_warn("invalid qmfbufflen: %u", qmfbufflen);
        tsp->result = -119;
        return -1;
    }	
    ebp = eservice_buf_new(qmfbufflen);
    eservice_buf_add(ebp, pkg_buf, qmfbufflen);

    if (tsp->protocol_type == 2) {
        char http_length_buffer[MAX_LINE_LEN];
        int snlen = snprintf(http_length_buffer, sizeof(http_length_buffer) - 1, 
                             "%zd\r\n\r\n", eservice_buf_datalen(ebp));
        eservice_buf_prepend(ebp, http_length_buffer, snlen);
        eservice_buf_prepend(ebp, http_fake_header, http_fake_header_len); 
    }

    if (tsp->client_port) prepend_proxy_header(tsp, ebp);

    ss_list_push_head_struct(&(tsp->output_ebp_list), eservice_buf_buf2list(ebp));
    return 0;
}

static int 
wns_staticis_connection_and_recived_num(eservice_unit_t *eup, wns_acc_task_struct *tsp, const QMF_PROTOCAL::QmfHead _qmf_head)
{
    wns_acc_connect_user_context_t *user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);

    if(user_context->datacomed == 0){
        if(tsp->protocol_type == 1){
            wns_connection_statisic.bin_client_data_recved ++;
            if (bit_test((int32_t *)&_qmf_head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_PING)) {
                wns_connection_statisic.bin_pin_connection_num ++;
            }
            else{
                wns_connection_statisic.bin_comm_connection_num ++;
            }
        }
        else if(tsp->protocol_type == 2){
            wns_connection_statisic.http_client_data_recved ++;
            if (bit_test((int32_t *)&_qmf_head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_PING)) {
                wns_connection_statisic.http_pin_connection_num ++;
            }
            else{
                wns_connection_statisic.http_comm_connection_num ++;
            }
        }
    }
    else
    {
        if(tsp->protocol_type == 1){
            wns_connection_statisic.bin_client_data_recved ++;
        }
        else if(tsp->protocol_type == 2){
            wns_connection_statisic.http_client_data_recved ++;
        }
    }
    return 0;
}
static int
wns_acc_process_pkg(eservice_unit_t *eup, wns_acc_task_struct *tsp)
{
    int res = -255;
    ssize_t input_len = 0;
    QMF_PROTOCAL::QmfHead _qmf_head;
    QMF_PROTOCAL::QmfHeadParser m_protocal_parser;
    uint8_t *data_start = NULL;
    int _acc_head_len = 0;
    QMF_PROTOCAL::QmfAccHead _acc_head;
    eservice_buf_t *send_ebp = NULL;
    eservice_buf_t *to_dispatch_ebp = NULL;
    char new_qmf_head_buf[MAX_LINE_LEN];
    int new_qmf_head_buf_len = sizeof(new_qmf_head_buf);

    size_t send_len = 0;
    wns_acc_connect_user_context_t *user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
    uint32_t i, statistic_port = 0;

    if (!tsp || !(tsp->input_ebp))
        return -1;

    g_dbg("=> wns_acc_process_pkg(), tsp: %p, input ebp: %p", tsp, tsp->input_ebp);

    input_len = eservice_buf_datalen(tsp->input_ebp);
    eservice_buf_make_linearizes(tsp->input_ebp, input_len);

    g_dbg("Data Come from Client(%s:%d), clientid=%lu, len=%zd", 
          eservice_user_get_peer_addr_str(eup), eservice_user_get_peer_addr_port(eup), tsp->client_id, input_len);

    /**************Deco QmfHead***************/
    _qmf_head.init();
    data_start = eservice_buf_start(tsp->input_ebp);
    res = m_protocal_parser.DecodeQmfHead((const char*)data_start, (unsigned int)input_len, _qmf_head);
    if (res != 0) {
        char _printhex[256+1];
        int _buflen = input_len > 128 ? 128 : input_len;
        HexToStr((const char *)data_start, _buflen, _printhex);
        g_warn("Decode QmfHead ERR. ret=%d. Client(%s:%d). Data=\n%s",
               res, eservice_user_get_peer_addr_str(eup), eservice_user_get_peer_addr_port(eup), _printhex);
        return res;
    }

    g_msg("decode QmfHead[len=%d ver=%d enc=%d appid=%d uin=%lu b2len=%zu bodylen=%d flag=%d seq=%d]",
          _qmf_head.Len, _qmf_head.Ver, _qmf_head.Enc, _qmf_head.Appid, _qmf_head.Uin, 
          _qmf_head.B2.size(), _qmf_head.BodyLen, _qmf_head.Flag, _qmf_head.seq);

    //statisic connection type  tcp http and ping connection
    wns_staticis_connection_and_recived_num(eup, tsp, _qmf_head);

    /* preserve connection last qua */
    if (user_context) {
        user_context->datacomed = 1;
        //eservice_user_conn_set_user_data(eup, user_context);
    }

    /**************Check Ping Package*********/
    if (bit_test((int32_t *)&_qmf_head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_PING)) {
        g_dbg("receive a ping package.");

        if (tsp->protocol_type == 2) {
            char http_length_buffer[MAX_LINE_LEN];
            int snlen = snprintf(http_length_buffer, sizeof(http_length_buffer) - 1, 
                                 "%zd\r\n\r\n", eservice_buf_datalen(tsp->input_ebp));
            eservice_buf_prepend(tsp->input_ebp, http_length_buffer, snlen);
            eservice_buf_prepend(tsp->input_ebp, http_fake_header, http_fake_header_len); 
        }
        // 如果是oc点加速过来的,需要加上相应的头
        if (tsp->client_port != 0) prepend_proxy_header(tsp, tsp->input_ebp);

        ss_list_push_head_struct(&(tsp->output_ebp_list), eservice_buf_buf2list(tsp->input_ebp));
        tsp->input_ebp = NULL;

        return 0;
    }

    if (_qmf_head.BodyLen == 0)
    {
        g_warn("receive a NOT-PING package with qmfhead.bodylen==0 uin=%lu qua=%s",
               _qmf_head.Uin, (user_context ? user_context->last_qua.c_str() : ""));

        return wns_pkg_error_data(eup, tsp, m_protocal_parser, _qmf_head, WNS_CODE_ACC_SERVER_ERROR_DATA);
    }

    /* check if appid is valid */
#if 0	
    if (wns_acc_valid_appid_map.find(_qmf_head.Appid) == wns_acc_valid_appid_map.end()) {
        g_warn("invalid appid: %d", _qmf_head.Appid);
        return -1;
    }
#endif

    /**************Build AccHead**************/

    _acc_head.init();
    _acc_head.Ver           = 0x03;				/* wan ip need version 3 or up */
    _acc_head_len = m_protocal_parser.GetAccHeadLen(_acc_head);
    _acc_head.SOH           = QMF_ACC_HEAD_SOH;
    _acc_head.ClientIP      = tsp->client_ip > 0 ? htonl(tsp->client_ip) : eservice_user_get_peer_addr_network(eup);
    _acc_head.ClientPort	= htons(eservice_user_get_peer_addr_port(eup));
    _acc_head.ClientID      = htonll(tsp->client_id);	
    //_acc_head.AccIp       = eth1_ip;
    //_acc_head.AccPort	= htons(eservice_user_get_related_port(eup));

    _acc_head.Len           = htonl(_acc_head_len + _qmf_head.Len);
    _acc_head.Ticks         = htonll(get_tick_count());

    _acc_head.Flag = WNS_FLAG_ORIGIN_DATA;			/* decrypt and decompress */

    if (tsp->is_akamai > 0) {
        _acc_head.Flag |= WNS_FLAG_AKAMAI_BIT;
    }

    if (tsp->protocol_type == 2) {
        _acc_head.Flag |= WNS_FLAG_HTTP_BIT;
    }

#if 0
    /* port statistic */
    statistic_port = eservice_user_get_related_port(eup);
    for (i = 0; i < WNS_ACC_MAX_LISTEN_PORT; i++) {
        if (wns_30s.port_statistic_array[i].used == 0) {
            wns_30s.port_statistic_array[i].used = 1;
            wns_30s.port_statistic_array[i].port = statistic_port;

            if (tsp->protocol_type == 1) {
                wns_30s.port_statistic_array[i].bin_prot_num = 1;
                wns_30s.port_statistic_array[i].http_prot_num = 0;
            }
            else if (tsp->protocol_type == 2) {
                wns_30s.port_statistic_array[i].bin_prot_num = 0;
                wns_30s.port_statistic_array[i].http_prot_num = 1;
            }

            break;
        }
        else {
            if (wns_30s.port_statistic_array[i].port == statistic_port) {
                if (tsp->protocol_type == 1) {
                    wns_30s.port_statistic_array[i].bin_prot_num++;
                }
                else if (tsp->protocol_type == 2) {
                    wns_30s.port_statistic_array[i].http_prot_num++;
                }

                break;
            }
        }
    }
#endif

    res = wns_acc_LogUpstreamInfo(eup, _qmf_head, _acc_head, tsp, &to_dispatch_ebp);
    if (res < 0) {
        ESERVICE_DESTROY_EBP(to_dispatch_ebp);

        if (tsp->error_code == WNS_CODE_ACC_DECRYPT_INVAID || tsp->error_code == WNS_CODE_ACC_INVALID_SESSIONHASH) {
            return wns_pkg_error_data(eup, tsp, m_protocal_parser, _qmf_head, tsp->error_code);
        }
        else {
            return -1;
        }
    }
    else if (res > 0) {
        return 0;
    }
    /******LOG SAC******/
    if(g_wns_acc_main_config.sac_agent_flag)
    {
        res = wns_acc_sacagent_check(eup, &_acc_head, &_qmf_head, tsp);
        if(res < 0){
            g_dbg("wns_acc_sacagent_check server failed\n");
        }
        else if(res == 0){
            g_dbg("wns_acc_sacagent_check no limited\n");
        }
        else{
            if (g_wns_acc_main_config.sac_agent_flag == 2){
                ESERVICE_DESTROY_EBP(to_dispatch_ebp);
                g_msg("wns_acc_sacagent_check limited %d uin:%u\n", res, tsp->client_info_p->uin);
                wns_acc_logSession(WNS_CODE_SAC_AGENT, eup, &_acc_head, &_qmf_head, tsp, 1);
                return wns_pkg_error_data(eup, tsp, m_protocal_parser, _qmf_head, WNS_CODE_SAC_AGENT );
            }
            else{
                g_msg("wns_acc_sacagent_check limited and only log :%u\n", tsp->client_info_p->uin);
            }
        }	
    }

#if	0
    if ((tsp->client_info_p->uin == 1463818982u || tsp->client_info_p->uin == 2202044200u) && strstr(tsp->client_info_p->cmd.c_str(), "addComment") && tsp->client_info_p->seq % 2 == 1) {
        ESERVICE_DESTROY_EBP(to_dispatch_ebp);

        res = wns_make_empty_response(tsp, WNS_CODE_ACC_NEED_RETRY);
        if (res < 0) {
            g_warn("wns_make_empty_response() failed: %d", res);
            return -1;
        }

        return 0;
    }
#endif

    //g_dbg("push special ip: %u, port: %u", process_spe_listen_ip, process_spe_listen_port);
    _acc_head.AccIp       = process_spe_listen_ip;
    _acc_head.AccPort	= htons(process_spe_listen_port);					/* special process port, 27000 - 27007 */
    _acc_head.service_port	= htons(eservice_user_get_related_port(eup));		/* service port, 80, 8080, 14000 */
    _acc_head.AccIp_wan = eservice_user_get_local_addr_network(eup);
    g_dbg("accip_wan = %u", _acc_head.AccIp_wan);

    if (tsp->client_info_p && !strncmp(tsp->client_info_p->cmd.c_str(), "wns.echo", 8)) {	/* echo, clog and return only */
        g_warn("receive a echo package, size: %zu", eservice_buf_datalen(tsp->input_ebp));

        if (tsp->protocol_type == 2) {
            char http_length_buffer[MAX_LINE_LEN];
            int snlen = snprintf(http_length_buffer, sizeof(http_length_buffer) - 1, 
                                 "%zd\r\n\r\n", eservice_buf_datalen(tsp->input_ebp));
            eservice_buf_prepend(tsp->input_ebp, http_length_buffer, snlen);
            eservice_buf_prepend(tsp->input_ebp, http_fake_header, http_fake_header_len); 
        }
        prepend_proxy_header(tsp, tsp->input_ebp);

        ss_list_push_head_struct(&(tsp->output_ebp_list), eservice_buf_buf2list(tsp->input_ebp));
        tsp->input_ebp = NULL;

        wns_acc_logSession(WNS_CODE_SUCC, eup, &_acc_head, &_qmf_head, tsp, 1);

        return 0;
    }

    /* preserve connection last qua */
    if (user_context) {
        user_context->last_qua = tsp->client_info_p->qua;
        //eservice_user_conn_set_user_data(eup, user_context);
    }

    //g_dbg("AccHead Len=%d; QmfHead Len=%d", _acc_head_len, _qmf_head.Len);

    if (tsp->need_not_rsp == 0)
        wns_30s.req_num++;

#if 0
    send_len = ntohl(_acc_head.Len);
    send_ebp = eservice_buf_new(0);
    if (!send_ebp) {
        g_warn("eservice_buf_new() failed");
        return -119;
    }

    if (eservice_buf_add(send_ebp,  &_acc_head, _acc_head_len) < 0 || eservice_buf_move_data(send_ebp, tsp->input_ebp, -1) < 0) {
        g_warn("eservice_buf_add() failed: %s", strerror(errno));
        ESERVICE_DESTROY_EBP(send_ebp);
        return -1;
    }

    res = wns_connect_to_dispatch_and_send_data(eup, tsp, send_ebp);
    if (res < 0) {
        g_warn("wns_connect_to_dispatch_and_send_data() failed: %d", res);
        ESERVICE_DESTROY_EBP(send_ebp);
        return res;
    }
#else
    m_protocal_parser.EncodeQmfHead(new_qmf_head_buf, &new_qmf_head_buf_len, _qmf_head, NULL, _qmf_head.BodyLen);
    new_qmf_head_buf_len -= _qmf_head.BodyLen;			/* we only need qmf header */

    if (eservice_buf_prepend(to_dispatch_ebp,  new_qmf_head_buf, new_qmf_head_buf_len) < 0) {
        g_warn("eservice_buf_add() failed: %s", strerror(errno));
        ESERVICE_DESTROY_EBP(to_dispatch_ebp);
		wns_acc_logSession(WNS_CODE_ACC_SEND2_DISP_FAILED, eup, NULL, NULL, NULL, 1);
        return -1;
    }

    _acc_head.Len           = htonl(eservice_buf_datalen(to_dispatch_ebp) + _acc_head_len);
    if (eservice_buf_prepend(to_dispatch_ebp,  &_acc_head, _acc_head_len) < 0) {
        g_warn("eservice_buf_add() failed: %s", strerror(errno));
        ESERVICE_DESTROY_EBP(to_dispatch_ebp);
		 wns_acc_logSession(WNS_CODE_ACC_SEND2_DISP_FAILED, eup, NULL, NULL, NULL, 1);
        return -1;
    }

    res = wns_connect_to_dispatch_and_send_data(eup, tsp, to_dispatch_ebp);
    if (res < 0) {
        g_warn("wns_connect_to_dispatch_and_send_data() failed: %d", res);
        ESERVICE_DESTROY_EBP(to_dispatch_ebp);
		wns_acc_logSession(WNS_CODE_ACC_DISPACH_SND_ERROR, eup, &_acc_head, &_qmf_head, tsp, 1);
		
        return wns_pkg_error_data(eup, tsp, m_protocal_parser, _qmf_head, WNS_CODE_ACC_DISPACH_SND_ERROR );
    }

#endif
    if (tsp->need_not_rsp == 0) {
        wns_30s.dispatch_req_num++;

        g_dbg("async tsp save, %lu -> %p, eup: %p real eup: %p", 
              tsp->client_id, tsp, eservice_user_eup_from_index(get_eup(tsp->client_id)), eup);
        wns_async_map[tsp->client_id] = (void *)tsp;
    }

    return 0;
}

/* cb_init should can be dynamic-reloaded */
extern "C" 
eservice_callback_return_val eservice_cb_init(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    int ret ;
    CFileConfig config; // = *CSingleton<CFileConfig>::instance();
    int res = -255;
    g_dbg("=> eservice_cb_init(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    if (config.ParseFile((const char *)garg)) {
        g_warn("parse config failed!\n");
        return eservice_cb_failed;
    }
    g_wns_acc_main_config.sac_agent_flag = 0;
    g_wns_acc_main_config.task_num = (uint32_t)atoi(config["server.task_num"].c_str());
    g_wns_acc_main_config.dest_modid = atoi(config["server.dest_modid"].c_str());
    g_wns_acc_main_config.dest_cmd = atoi(config["server.dest_cmd"].c_str());

    g_wns_acc_main_config.other_dest_modid = atoi(config["server.other_dest_modid"].c_str());
    g_wns_acc_main_config.other_dest_cmd = atoi(config["server.other_dest_cmd"].c_str());

    g_wns_acc_main_config.client_retry = atoi(config["server.client_retry"].c_str());
    g_wns_acc_main_config.idle_clean_seconds = atoi(config["server.idle_clean_seconds"].c_str());
    g_wns_acc_main_config.fast_idle_clean_seconds = atoi(config["server.fast_idle_clean_seconds"].c_str());
    g_wns_acc_main_config.highest_idle_clean_seconds = atoi(config["server.highest_idle_clean_seconds"].c_str());
    g_wns_acc_main_config.firstnodata_idle_clean_seconds = atoi(config["server.firstnodata_idle_clean_seconds"].c_str());
    g_wns_acc_main_config.sac_agent_flag = atoi(config["server.sac_agent_flag"].c_str());
    g_sac_dcagent_flag = atoi(config["server.sac_dcagent_flag"].c_str());
    g_wns_acc_main_config.clean_clog = atoi(config["server.clean_clog"].c_str());
    g_wns_acc_main_config.max_pkg_size = atoi(config["server.max_pkg_size"].c_str());

    if (g_wns_acc_main_config.idle_clean_seconds <= 0)
        g_wns_acc_main_config.idle_clean_seconds = WNS_ACC_DEFAULT_CONN_IDLE_SECONDS;
    if (g_wns_acc_main_config.fast_idle_clean_seconds <= 0)
        g_wns_acc_main_config.fast_idle_clean_seconds = WNS_ACC_DEFAULT_CONN_IDLE_SECONDS;
    if (g_wns_acc_main_config.highest_idle_clean_seconds <= 0)
        g_wns_acc_main_config.highest_idle_clean_seconds = WNS_ACC_DEFAULT_CONN_IDLE_SECONDS;
    if (g_wns_acc_main_config.firstnodata_idle_clean_seconds <= 0)
        g_wns_acc_main_config.firstnodata_idle_clean_seconds = WNS_ACC_DEFAULT_CONN_IDLE_SECONDS;

	CFileConfig cookie_map_config;
	cookie_map_config.ParseFile("../conf/eservice_map.ini");
	g_wns_cookie_map = cookie_map_config.GetMap();

	map<string ,string>::iterator iter = g_wns_cookie_map.begin();
	for(; iter != g_wns_cookie_map.end() ; ++ iter)
	{
		g_dbg("first:%s,second:%s", iter->first.c_str(), iter->second.c_str());
	}

    g_dbg("task num: %u, dest modid: %d, dest cmd: %d, idle clean seconds: %d", g_wns_acc_main_config.task_num, g_wns_acc_main_config.dest_modid, g_wns_acc_main_config.dest_cmd, g_wns_acc_main_config.idle_clean_seconds);
    g_dbg("clean_clog flag:%d",g_wns_acc_main_config.clean_clog);
    g_dbg("other_dest modid: %d, other_dest cmd: %d max_pkg:%d",g_wns_acc_main_config.other_dest_modid,g_wns_acc_main_config.other_dest_cmd,g_wns_acc_main_config.max_pkg_size);
    g_dbg("init ok");

    wns_acc_valid_appid_map.clear();
    
    wns_acc_valid_appid_map[65537] = 1;	/* Qpai */
    wns_acc_valid_appid_map[65538] = 1;	/* Qzone */
    wns_acc_valid_appid_map[65539] = 1;	/* Qzone outer */
    wns_acc_valid_appid_map[1000002] = 1;	/* open platform android SDK */
    wns_acc_valid_appid_map[1000003] = 1;	/* Qzone DB */
    wns_acc_valid_appid_map[1000004] = 1;	/* photo manager */
    wns_acc_valid_appid_map[1000005] = 1;	/* Valentine space */
    wns_acc_valid_appid_map[1000006] = 1;	/* secretry box */

    return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_pre_run(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    struct timeval one_sec_timer_to = {1, 0};
    struct timeval statistic_timer_to = {30, 0};

    int res = -255;
    in_addr_t the_eth1_ip;
    struct in_addr the_ip;

    g_dbg("=> eservice_cb_pre_run(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    special_get_response_len = strlen(special_get_response_str);
    http_fake_header_len = strlen(http_fake_header);

    /* eth1 address */
    res = get_addr_from_device("eth1", &the_eth1_ip);
    if (res < 0) {
        g_warn("get_addr_from_device() failed: %d", res);
        return eservice_cb_failed;
    }
    eth1_ip = (network32_t)the_eth1_ip;

    memset(&wns_30s, 0, sizeof(wns_30s));
    memset(&wns_connection_statisic, 0 ,sizeof(wns_connection_statisic));
    memset(&wns_idle_statisic, 0 ,sizeof(wns_idle_statisic));
    memset(&wns_user_distribute, 0 ,sizeof(wns_user_distribute));

    /* init task struct list */
    if (wns_acc_init_task_struct_list(g_wns_acc_main_config.task_num, &g_min_wns_acc_tsp_address_val, &g_max_wns_acc_tsp_address_val) < 0)
        g_err("wns_acc_init_task_struct_list() failed");

    /* 30s timer */
    memset(&statistic_timer, 0, sizeof(statistic_timer));
    statistic_timer_cb(0, 0, egr);

    /* 1s timer */
    memset(&one_sec_timer, 0, sizeof(one_sec_timer));
    one_sec_timer_cb(0, 0, egr);

    return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_after_accept(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    wns_acc_connect_user_context_t *user_context = NULL;

    g_dbg("=> eservice_cb_after_accept(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);

    delete_dbg(user_context);
    new_dbg(user_context, wns_acc_connect_user_context_t);
    user_context->session_hash = 0;
    user_context->datacomed = 0;
    eservice_user_conn_set_user_data(eup, (void *)user_context);

    return eservice_cb_success;
}

#if 0
extern "C" 
eservice_callback_return_val eservice_cb_listen_timeout(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    g_dbg("=> eservice_cb_listen_timeout(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    return eservice_cb_success;
}
#endif

static int
eservice_user_distribute_staticis(eservice_unit_t *eup)
{
    if(eservice_user_conn_idle_seconds(eup) >= 900 && eservice_user_conn_idle_seconds(eup) < 1200){
        wns_user_distribute.min15_20_user_ilde_num++;
    }
    else if (eservice_user_conn_idle_seconds(eup) >= 600 && eservice_user_conn_idle_seconds(eup) < 900){
        wns_user_distribute.min10_15_user_ilde_num++;
    }
    else if (eservice_user_conn_idle_seconds(eup) >= 300 && eservice_user_conn_idle_seconds(eup) < 600){
        wns_user_distribute.min5_10_user_ilde_num++;
    }
    else if (eservice_user_conn_idle_seconds(eup) >= 0 && eservice_user_conn_idle_seconds(eup) < 300){
        wns_user_distribute.min0_5_user_ilde_num++;
    }
    return 0;

}

extern "C" 
eservice_callback_return_val eservice_cb_tcp_conn_timeout(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    wns_acc_connect_user_context_t *user_context = NULL;
    user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
    /*
       g_dbg("=> eservice_cb_tcp_conn_timeout(), "
       "eup: %p, earg: %p, garg: %p g_total_connection_num: %ld  g_max_connect:%d",
       eup, earg, garg, g_total_connection_num, g_max_connection_num);
       g_dbg("=> eservice_cb_tcp_conn_timeout(), datecomde:%d", user_context->datacomed);
       */

    //staticis user distribute
    eservice_user_distribute_staticis(eup);

    //first data had not came
    if(user_context->datacomed == 0){
        if (eservice_user_conn_idle_seconds(eup) >= g_wns_acc_main_config.firstnodata_idle_clean_seconds){
            string client_ip = TOOL::Int2Str((eservice_user_get_peer_addr_network(eup)));
            g_warn("client connection first not datacome cut, eup: %p user_context ip:%s", eup, client_ip.c_str());
            wns_idle_statisic.first_no_data_num ++;
            wns_acc_logSession(WNS_CODE_FIRST_NO_DATA, eup, NULL, NULL, NULL);
            goto cleaneup;
        }
    }
    //overload is ok 
    if (eservice_user_conn_idle_seconds(eup) >= g_wns_acc_main_config.idle_clean_seconds) {
        g_dbg("client connection timeout cut, eup: %p", eup);
        wns_idle_statisic.comm_idle_time_num ++;
        goto cleaneup;
    }
    //above 95% overload ,and entry out of services overloads ,beginning highest clean idle eup
    if (g_total_connection_num > (g_max_connection_num  * 95 / 100)){
        if(eservice_user_conn_idle_seconds(eup) >= g_wns_acc_main_config.highest_idle_clean_seconds){
            g_dbg("client connection highest timeout cut, eup: %p", eup);
            wns_idle_statisic.highest_idle_time_num ++;
            wns_acc_logSession(WNS_CODE_OVERLOAD_ACTIVE_CLOSE, eup, NULL, NULL, NULL);
            goto cleaneup;
        }
    }
    //above 85% overload ,and entry high overloads ,beginning fast clean idle eup
    if (g_total_connection_num > (g_max_connection_num  * 85 / 100)){
        if(eservice_user_conn_idle_seconds(eup) >= g_wns_acc_main_config.fast_idle_clean_seconds){
            g_dbg("client connection fast timeout cut, eup: %p", eup);
            wns_idle_statisic.fast_idle_time_num ++;
            wns_acc_logSession(WNS_CODE_OVERLOAD_ACTIVE_CLOSE, eup, NULL, NULL, NULL);
            goto cleaneup;
        }
    }
    return eservice_cb_success;
cleaneup:
    delete_dbg(user_context);
    eservice_user_conn_set_user_data(eup, NULL);
    eservice_user_destroy_tcp_conn(eup);
    return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_tcp_terminate(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    wns_acc_connect_user_context_t *user_context = NULL;

    g_dbg("=> eservice_cb_tcp_terminate(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    user_context = (wns_acc_connect_user_context_t *)eservice_user_conn_get_user_data(eup);
    delete_dbg(user_context);
    eservice_user_conn_set_user_data(eup, NULL);

    return eservice_cb_success;
}

extern "C" 
eservice_callback_return_val eservice_cb_data_arrive(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg)
{
    struct eservice_buf_t *pkg_data = NULL;
    ssize_t pkg_len = 0, buf_data_len = 0, http_header_len = 0;
    int res = -255, is_akamai = 0, is_cdn = 0;
    wns_psy_header_t *psy_headp = NULL;
    wns_cdnproxy_psy_header_t *cdnproxy_header = NULL;
    wns_acc_task_struct *tsp = NULL;
    uint8_t *ptr = NULL;
    char *endp = NULL;
    host32_t client_ip = 0;
    uint16_t client_port = 0;
    struct eservice_buf_t * the_ebp = NULL;
    struct dbl_list_head *dptr = NULL;
    eservice_callback_return_val push_ret = eservice_cb_data_arrive_fatal;
    wns_acc_connect_user_context_t *user_context = NULL;
    eservice_unit_t *client_eup = NULL;

    g_dbg("=> eservice_cb_data_arrive(), eup: %p, earg: %p, garg: %p", eup, earg, garg);

    while (1) {
        pkg_data = NULL;
        pkg_len = 0;
        tsp = NULL;
        is_akamai = 0;

        buf_data_len = eservice_user_datalen(eup);
        if (buf_data_len < 0) {
			wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
            return eservice_cb_data_arrive_fatal;
        }

        if (buf_data_len < (ssize_t)sizeof(wns_psy_header_t) || buf_data_len > WNS_ACC_MAX_VALID_PKG_LEN) {
            return eservice_cb_data_arrive_not_enough;
        }

        /*find if it is cndproxy_header*/
//        cdnproxy_header = (wns_cdnproxy_psy_header_t *)(eservice_user_pos_ex(eup, 0, (ssize_t)(sizeof(wns_cdnproxy_psy_header_t))));
//        if (cdnproxy_header == NULL)
//        {
//            g_warn("user_pos_ex return NULL");
//            return eservice_cb_data_arrive_fatal;
//        }
//        if (cdnproxy_header->Version != (char)0x82) {
//            /* parse protocol */
//            cdnproxy_header = NULL;
//            g_msg("wns_psy_header");
//        }
//        else{
//            client_ip = ntohl(cdnproxy_header->User_ip);
//            client_port = ntohs(cdnproxy_header->User_port);
//            g_msg("cdnproxy_header clientipnum: %d ip:port:%s:%d",
//                  cdnproxy_header->User_ip, TOOL::Int2Str(cdnproxy_header->User_ip),
//                  client_port);
//
//            eservice_user_drain(eup, (ssize_t)(sizeof(wns_cdnproxy_psy_header_t)));
//        }

        psy_headp = (wns_psy_header_t *)(eservice_user_pos_ex(eup, 0, (ssize_t)(sizeof(wns_psy_header_t))));
        if (psy_headp == NULL)
        {
            g_warn("user_pos_ex return NULL");
            return eservice_cb_data_arrive_fatal;
        }

        if (psy_headp->wns_protocol.Wns_sign == WNS_SIGN_INT) {
            pkg_len = ntohl(psy_headp->wns_protocol.Len);
            g_msg("wns protocol, len: %zd", pkg_len);

            if (pkg_len <= (ssize_t)(sizeof(wns_psy_header_t)) || pkg_len > WNS_ACC_MAX_VALID_PKG_LEN) {
                g_warn("invalid wns pkg len: %zd", pkg_len);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                return eservice_cb_data_arrive_fatal;
            }

            if (buf_data_len < pkg_len) {
                eservice_user_next_need(eup, pkg_len - buf_data_len);
                return eservice_cb_data_arrive_not_enough;
            }

            res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
            if (res < 0 || !pkg_data) {
                g_warn("eservice_user_pop_inbuf() failed: %d", res);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                return eservice_cb_data_arrive_fatal;
            }

            tsp = wns_acc_alloc_task_struct();
            if (!tsp) {
                g_warn("wns_acc_alloc_task_struct() failed");
                ESERVICE_DESTROY_EBP(pkg_data);
                continue;
            }

            tsp->eup = eup;
            tsp->input_ebp = pkg_data;

            int idx = eservice_user_eup_to_index(eup);
            if (idx < 0)
            {
                g_warn("eservice_user_eup_to_index() failed, eup=%p", eup);
                //ESERVICE_DESTROY_EBP(pkg_data);
                wns_acc_free_task_struct(tsp);
                continue;
            }

            tsp->client_id = make_client_id(idx);
            tsp->protocol_type = 1;
            tsp->client_ip = client_ip;
            tsp->client_port = client_port;

            /* process */
            res = wns_acc_process_pkg(eup, tsp);
        }
        //心跳逻辑处理
        else if (psy_headp->wns_fe_protocol.Wns_fe_sign == WNS_SIGN_FE_INT) {			/* sepcial fe protocol, only 4 bytes, echo */
            g_msg("wns fe protocol");

            res = eservice_user_pop_inbuf(eup, WNS_FE_PROTOCOL_LEN, &pkg_data);
            if (res < 0 || !pkg_data) {
                g_warn("eservice_user_pop_inbuf() failed: %d", res);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                return eservice_cb_data_arrive_fatal;
            }

            //g_dbg("set tcp no delay");
            //eservice_user_set_tcp_nodelay(eup, 1);
            eservice_user_push_outbuf(eup, pkg_data);
            //g_dbg("clear tcp no delay");
            //eservice_user_set_tcp_nodelay(eup, 0);
        }
        else if (!strncmp(psy_headp->wns_post_protocol.post, "POST", 4)) {}
        else if (!strncmp(psy_headp->wns_get_protocol.get, "GET", 3)) {}
        //广播逻辑处理
        else if (psy_headp->wns_push_protocol.SOH == STX_INNER_SIGN) {				/* push server data */
            client_eup = NULL;

            pkg_len = ntohl(psy_headp->wns_push_protocol.Len);
            g_msg("wns push protocol, len: %zd", pkg_len);

            if (pkg_len <= (ssize_t)(sizeof(wns_inner_psy_header_t)) || pkg_len > WNS_ACC_MAX_VALID_PKG_LEN) {
                g_warn("invalid wns inner pkg len: %zd", pkg_len);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                return eservice_cb_data_arrive_fatal;
            }

            if (buf_data_len < pkg_len) {
                eservice_user_next_need(eup, pkg_len - buf_data_len);
                return eservice_cb_data_arrive_not_enough;
            }

            res = eservice_user_pop_inbuf(eup, pkg_len, &pkg_data);
            if (res < 0 || !pkg_data) {
                g_warn("eservice_user_pop_inbuf() failed: %d", res);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                return eservice_cb_data_arrive_fatal;
            }

            tsp = wns_acc_alloc_task_struct();
            if (!tsp) {
                g_warn("wns_acc_alloc_task_struct() failed");
                ESERVICE_DESTROY_EBP(pkg_data);
                continue;
            }

            tsp->input_ebp = pkg_data;
            tsp->protocol_type = 4;

            /* process */
            res = wns_acc_get_push_client(eup, &client_eup, tsp);

            tsp->eup = client_eup;
            int idx = eservice_user_eup_to_index(eup);
            if (idx < 0)
            {
                g_warn("eservice_user_eup_to_index() failed, eup=%p", eup);
                // ESERVICE_DESTROY_EBP(pkg_data); // will free when free tsp
                wns_acc_free_task_struct(tsp);
                continue;
            }
            tsp->client_id = make_client_id(idx);
        }
        else {
            g_warn("unknown protocol, [%x %x %x %x]", 
                   psy_headp->wns_post_protocol.post[0], 
                   psy_headp->wns_post_protocol.post[1], 
                   psy_headp->wns_post_protocol.post[2], 
                   psy_headp->wns_post_protocol.post[3]);
            return eservice_cb_data_arrive_fatal;
        }

        if (res != 0) {
            g_warn("wns_acc_process_pkg() failed: %d", res);

            if (tsp && tsp->result == -119) {
                wns_acc_free_task_struct(tsp);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                return eservice_cb_data_arrive_fatal;
            }
            else {
                wns_acc_free_task_struct(tsp);
				wns_acc_logSession(WNS_CODE_ACC_SERVER_ERROR_DATA, eup, NULL, NULL, NULL, 1);
                continue;
            }
        }

        if (tsp && tsp->output_ebp_list.number > 0) {
            dptr = ss_list_pop_head_struct(&(tsp->output_ebp_list));
            if (dptr) {
                the_ebp = eservice_buf_list2buf(dptr);
                eservice_buf_set_control(the_ebp, 1, pkg_send_failed_cb, NULL);

                if (client_eup) {
                    res = eservice_user_push_outbuf(client_eup, the_ebp);
                }
                else {
                    res = eservice_user_push_outbuf(eup, the_ebp);
                }
                if (res < 0) {
                    g_warn("eservice_user_push_outbuf() failed: %d", res);
                }
            }

            wns_acc_free_task_struct(tsp);
            continue;
        }

        if (tsp && tsp->need_not_rsp == 0) {
            wns_acc_add_waiting_task_struct(tsp);
        }
        else {
            wns_acc_free_task_struct(tsp);
        }
    }	

    return eservice_cb_data_arrive_ok;
}
