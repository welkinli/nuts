/*
 * qmf_comm_define.h
 *
 *  Created on: 2012-5-15
 *      Author: himzheng
 */

#ifndef QMF_COMM_DEFINE_H_
#define QMF_COMM_DEFINE_H_

#include "wns_errcode.h"

//内部系统使用qzone协议时，cmd的统一定义
enum QmfQzoneProtocalCmd
{
    QMF_PDU_CMD_UPSTREAM        = 0x01,//使用QmfUpstream结构体
    QMF_PDU_CMD_DOWNSTREAM      = 0x02,//使用QmfDownstream结构体
    QMF_PDU_CMD_PROVIDER        = 0x03,//使用QmfProvidorReq结构体
    QMF_PDU_CMD_NOBODY          = 0x100,//没有body 只是一个qzone头
    /**********PUSH************/
    WNS_PUSH_CMD_HEARTBEAT      = 101, //心跳
    WNS_PUSH_CMD_RESULT         = 102, //push终端回包
    WNS_PUSH_CMD_ONLINE         = 103, //用户上线注册
    WNS_PUSH_CMD_PROVIDE        = 104, //业务侧发送push
    WNS_PUSH_CMD_ADDQUE         = 105, //添加到发送队列
    WNS_PUSH_CMD_PUSH           = 106,
    WNS_PUSH_CMD_APNS           = 107, //指明发送IOS
    WNS_PUSH_CMD_LOGOUT         = 108, //注销
    WNS_PUSH_CMD_STATUS         = 109, //获取用户状态
    WNS_PUSH_CMD_LOG            = 110, //log控制
    WNS_PUSH_CMD_PROVIDE2       = 113, //新版push接口
    WNS_PUSH_CMD_STATUS2        = 114, //新版获取用户状态
    WNS_PUSH_CMD_MESSAGE        = 115, //获取用户消息
    WNS_PUSH_CMD_PROVIDE_BATCH  = 118, //批量发送push接口
    WNS_PUSH_CMD_MPNS           = 119, //指明发送win
    
    /*********wireless*********/
    WNS_PUSH_CMD_SYNC_LOGINSTAT = 111, //无线同步给我们登陆状态
    WNS_PUSH_CMD_CHK_NEEDPUSH   = 112, //无线到我们这里检查是否需要发送push
    
    /*********  互娱  *********/
    WNS_PUSH_CMD_CHECK_QZONEONLINE = 116, //判断用户qzone是否在线
    
    WNS_PUSH_CMD_RESEND = 117, //push补发
};

//WNS系統分配的APP
enum WnsAppid
{
    WNS_APPID_SUIPAI            = 0x10001,//Q拍
    WNS_APPID_QZONE             = 0x10002,//qzone
};

#define WNS_PUSH_UID_LEN 8

//命令字定义
#define QMF_SERVICE_CMD_LOGIN           "wns.login"
#define QMF_SERVICE_CMD_LOGOFF          "wns.logoff"
#define QMF_SERVICE_CMD_HEARTBEAT       "wns.heartbeat"
#define QMF_SERVICE_CMD_PUSH            "wns.push"
#define QMF_SERVICE_CMD_PUSHRSP         "wns.pushrsp"
#define QMF_SERVICE_CMD_PUSH_REGISTER   "wns.push.register"
#define QMF_SERVICE_CMD_HANKSHAKE       "wns.handshake"
#define QMF_SERVICE_CMD_SERVERLIST      "wns.serverlist"
#define QMF_SERVICE_CMD_SPEED4TEST      "wns.speed4test"
#define QMF_SERVICE_CMD_GETSETTING      "wns.setting"
#define QMF_SERVICE_CMD_REPORTLOG       "wns.reportlog"
#define QMF_SERVICE_CMD_FORCE_REPORTLOG "wns.forceReportLog"
#define QMF_SERVICE_CMD_DEVICEREPORT    "wns.deviceReport"
#define QMF_SERVICE_CMD_DEVICECUT       "wns.deviceCut"
#define QMF_SERVICE_CMD_PROVIDE         "wns.provide"
#define QMF_SERVICE_CMD_LOGCONTROL      "wns.logcontrol"
#define QMF_SERVICE_CMD_LOGUPLOAD       "wns.logupload"
#define QMF_SERVICE_CMD_GETCONFIG       "wns.getconfig"
#define QMF_SERVICE_CMD_GETTESTIP		"wns.gettestip"
//Q拍验票专用
#define QMF_SERVICE_CMD_QPAI_CHECK      "wns.qpai.check"

//模调
#define LS_WNS_LOGIN_SVR 204967170
#define LS_WNS_LOGIN_SVR_LOGIN 204967171
#define LS_WNS_LOGIN_SVR_CHECK 204967172
#define LS_WNS_LOGIN_SVR_PTLOGIN 204967173
#define LS_WNS_LOGIN_SVR_IPCHECK 204967188

//CMD
#define LS_WNS_CMDSVR 204968318
#define LS_WNS_CMDSVR_INNER 204968319
#define LS_IF_WNS_CMDSVR_HANDSHAKE 104923379
#define LS_IF_WNS_CMDSVR_SPEED4TEST 104923380
#define LS_IF_WNS_CMDSVR_REPORTLOG 104923381
#define LS_IF_WNS_CMDSVR_GETCONF 104923382
#define LS_IF_WNS_CMDSVR_GETCONF_VIA_HB 104923731

//ACC
#define LS_WNS_ACCESS 204967169
#define LS_WNS_ACCESS_DISPTACHER 204967179
#define LS_IF_WNS_ACCESS_TRANS 104922321

//PUSH
#define LS_WNS_PUSHSVR 204967217
#define LS_IF_WNS_PUSH_NEWUID 104922426


#define LS_IF_WNS_LOGIN_CHECKLOGIN 104922300
#define LS_IF_WNS_LOGIN_PTLOGIN 104922301
#define LS_IF_WNS_LOGIN_CHECKTOKEN 104922302
#define LS_IF_WNS_LOGIN_IPCHECK 104922336
#define LS_IF_WNS_LOGIN_SIDCHECK 104922423
#define LS_IF_WNS_LOGIN_SUIPAICHECK 104922445



#endif /* QMF_COMM_DEFINE_H_ */
