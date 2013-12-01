/*
 * @file wns_errcode.h
 * @brief WNS统一返回码
 * @author himzheng
 * @date 2012-7-16
 */

#ifndef WNS_ERRCODE_H_
#define WNS_ERRCODE_H_


typedef enum {

    WNS_CODE_SUCC                   = 0,//SUCC

    //1~999

    //acc 1000 ~ 1499
	WNS_CODE_ACC_SERVER_ACCUNPACK       = 1001, // 收到server回包解acc头失败
	WNS_CODE_ACC_SERVER_QMFUNPACK       = 1002, // 收到server回包解qmf头失败
	WNS_CODE_ACC_SERVER_INVAILDLEN      = 1003, // 收到server回包acclen错误
	WNS_CODE_ACC_SERVER_TIMEOUT         = 1004, // server回包超时
	WNS_CODE_ACC_DISPACH_SND_ERROR          = 1005,
	WNS_CODE_ACC_SERVER_ERROR_DATA      = 1006, //数据包不合法
	WNS_CODE_ACC_BACKSERVER_ERROR_DATA      = 1007, //后端数据包不合法
	WNS_CODE_ACC_SERVER_PROCESS_FAILED   = 1008, //acc处理错误依然回包给终端
	
	WNS_CODE_ACC_CLIENT_SND_ERR         = 1050, // client回包失败
	WNS_CODE_ACC_CLIENT_INVAID          = 1051, // client不存在
	WNS_CODE_ACC_DECRYPT_INVAID	= 1052, // 解密失败
	WNS_CODE_ACC_INVALID_SESSIONHASH = 1053, // session hash非法
	WNS_CODE_ACC_NEED_RETRY = 1054, // 前端重试
	WNS_CODE_ACC_SEND2_DISP_FAILED = 1055,  //发送到disp失败
	WNS_CODE_ACC_ERROR_EMPTY = 1056,        //回包error 打包空包给终端
	
    //loginsvr 1500 ~ 1999
    WNS_CODE_LOGIN_PTLOGIN_BUSY         = 1901,//ptlogin服务失败
    WNS_CODE_LOGIN_3GSVR_BUSY           = 1902,//3gkey服务失败
    WNS_CODE_LOGIN_A2_ILLEGAL           = 1903,//a2失效
    WNS_CODE_LOGIN_SID_ILLEGAL          = 1904,//sid非法
    WNS_CODE_LOGIN_SID_EXPIRED          = 1905,//sid过期
    WNS_CODE_LOGIN_A2_EXPIRED           = 1906,//a2超过有效期
    WNS_CODE_LOGIN_A2_CHANGED           = 1907,//用户修改密码导致失效
    WNS_CODE_LOGIN_NOTOKEN              = 1908,//没有任何可验证的token
    WNS_CODE_LOGIN_CREATEUIDFAIL        = 1909,//获取用户UID失败
    WNS_CODE_LOGIN_B2_EXPIRED           = 1910,//b2过期
    WNS_CODE_LOGIN_SID_VERICODE         = 1911,//sid需要弹验证码
    WNS_CODE_LOGIN_QUA_INVAILD          = 1912, // appid/qua无效
    WNS_CODE_LOGIN_NEEDCHANGE           = 1600,//往后端时需要更换key
    WNS_CODE_LOGIN_INTER_ERR            = 1550,//内部失败
    WNS_CODE_LOGIN_SPP_UNPACK           = 1551,//SPP解包失败
    WNS_CODE_LOGIN_SPP_PACK             = 1552,//SPP pack failed
    WNS_CODE_CMDSVR_NOACCIP             = 1501,//通用协议-无法获取accip
    WNS_CODE_CMDSVR_NOCLIENTIP          = 1502,//通用协议-无法获取clientip
    WNS_CODE_CMDSVR_INTER_ERR           = 1503,//通用协议-内部错误
    WNS_CODE_CMDSVR_UNPACK              = 1504,//通用协议-解包失败
    WNS_CODE_CMDSVR_PACK                = 1505,//通用协议-打包失败


    //dispatcher 2000 ~ 2999
    //client
    WNS_CODE_DIS_CLIENT_NEXTPACK        = 2101,//进包阶段-封下一阶段请求包失败
    WNS_CODE_DIS_CLIENT_ACCUNPACK       = 2102,//进包阶段-解acc失败
    WNS_CODE_DIS_CLIENT_NOROUTE         = 2103,//进包阶段-没有路由信息

    //auth
    WNS_CODE_DIS_AUTH_NEXTPACK          = 2201,//鉴权阶段-封下一阶段请求包失败
    WNS_CODE_DIS_AUTH_ACCUNPACK         = 2202,//鉴权阶段-解acc失败
    WNS_CODE_DIS_AUTH_RSPUNPACK         = 2203,//鉴权阶段-本阶段回包解包失败

    //api
    WNS_CODE_DIS_API_NEXTPACK           = 2301,//业务阶段-封下一阶段请求包失败（返回客户端包）
    WNS_CODE_DIS_API_ACCUNPACK          = 2302,//业务阶段-解acc失败
    WNS_CODE_DIS_API_RSPUNPACK          = 2303,//业务阶段-本阶段回包解包失败
	
    //proxy
    WNS_CODE_DIS_FORWORD_RSP            = 2304,//同步中心QZA返回失败
	WNS_CODE_DIS_WEBAPPS_OVERLOAD		= 2305,//后端业务过载
    //stat
    WNS_CODE_DIS_STAT_BEGIN             = 2400,//session异常阶段返回码，返回时加上stat

    //pushsvr 3000 ~ 3999
    WNS_CODE_PUSH_INTER_ERROR           = 3000,
    WNS_CODE_PUSH_UNPACK                = 3001,
    WNS_CODE_PUSH_PACK                  = 3002,
    WNS_CODE_PUSH_BUFF_LIMIT            = 3003,
    WNS_CODE_PUSH_NOMATCH_STATE         = 3004,
    WNS_CODE_PUSH_OVERLOAD              = 3005,
    WNS_CODE_PUSH_INVALIDE_UIN          = 3010,//uin invalide
    WNS_CODE_PUSH_SERVICECMD_NOT_REGIST = 3011,//servicecmd not regist
    WNS_CODE_PUSH_UID_NOT_EXIST         = 3012,// uid not belong 2 user
    WNS_CODE_PUSH_UID_ILLEGAL           = 3013,// request not contain uid
    WNS_CODE_PUSH_SEND_2_ACC_FAIL       = 3014,//send 2 acc fail
    WNS_CODE_PUSH_SEND_2_ACC_OFFLINE    = 3015,//client not online
    WNS_CODE_PUSH_NOMSG_2_SEND          = 3016,//no msg 2 send
    WNS_CODE_PUSH_QUA_INVALIDE          = 3017,//qua parse fail
    WNS_CODE_PUSH_ACC_OVERLOAD          = 3018,//acc overload

    WNS_CODE_PUSH_CACHE_NOT_INIT        = 3100,//providor not init
    WNS_CODE_PUSH_CACHE_GET_LIST_FAIL   = 3101,//tmem getlist fail
    WNS_CODE_PUSH_CACHE_SET_FAIL        = 3102,//tmem set token fail
    WNS_CODE_PUSH_CACHE_GET_FAIL        = 3103,//tmem get token fail

    WNS_CODE_PUSH_PUSHRSP_TIMEOUT       = 3201,
    WNS_CODE_PUSH_IOS_NO_DEVTOKEN       = 3202,


	/* security */
	WNS_CODE_SAFE_START = 4000,
	WNS_CODE_SAC_AGENT = 4001,                 //sac_check
	WNS_CODE_FIRST_NO_DATA = 4002,                 //frist no data
	WNS_CODE_OVERLOAD_ACTIVE_CLOSE = 4003,         //server overload active close fd

	WNS_CODE_SAFE_END = 4100,

	CMD_ERROR = 5000,
}ENUM_WNS_ERRCODE;

#endif /* WNS_ERRCODE_H_ */
