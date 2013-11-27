/*******************************************************************************
*
* 版权: (C) 2011-2020, 腾讯公司
* 文件: sac_agent.h
* 功能: 安全Agent头文件
* 作者: xuewu
* 版本: V1.0
* 修改历史: 1. Created by xuewu, 2012-03-07
*
********************************************************************************/
#ifndef  __sac__agent__h__
#define  __sac__agent__h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum  EN_DimExt
{
    DIM_QUA_CMD = 0x4000,
    DIM_CMD = 0x8000
};

// 维护维度Id, dimensionId
enum  EN_DimensionId
{
    APPID = 0x1,
    APPID_OPUIN = 0x2,
    APPID_TOUIN = 0x4,
    APPID_PARENTUIN = 0x8,
    APPID_IP = 0x10,
    APPID_OPOPENID = 0x20,
    APPID_QUA = 0x40,
    APPID_IMEI = 0x80,
    APPID_IPV6 = 0x100,
    APPID_CMD_OPUIN = 0x8002,
    APPID_CMD_TOUIN = 0x8004,
    APPID_CMD_PARENTUIN = 0x8008,
    APPID_CMD_IP = 0x8010,
    APPID_CMD_OPOPENID = 0x8020,
    APPID_CMD_QUA = 0x8040,
    APPID_CMD_IMEI = 0x8080,
    APPID_CMD_IPV6 = 0x8100,
    APPID_CMD_QUA_OPUIN = 0x4002,
    APPID_CMD_QUA_TOUIN = 0x4004,
    APPID_CMD_QUA_PARENTUIN = 0x4008,
    APPID_CMD_QUA_IP = 0x4010,
    APPID_CMD_QUA_OPOPENID = 0x4020,
    APPID_CMD_QUA_IMEI = 0x4080,
    APPID_CMD_QUA_IPV6 = 0x4100,
    DIMENSION_ALL = 0xFFFF      // 所有维度
};

typedef  struct __tagQueryCond
{
    unsigned int uiQueryDimensionId;  // 查询维度, 取值范围 EN_DimensionId, 多个维度用"|"连接使用
    unsigned int uiAppid;       // 应用Id, 必填
    unsigned int uiIpAddr;      // 操作者IP。主机字节序IP
    unsigned int uiOpUin;       // 操作者UIN
    unsigned int uiToUin;       // 被操作者UIN
    unsigned int uiParentUin;   // 日志或贴子原创者UIN
    char szOpOpenId[33];        // 操作者OpenId, 32字节可打印字符串, 根据 enQueryDimensionId 选填
    char szQua[513];            // qua 手机版本标识
    char szImei[33];            // imei 移动设备身份码标识
    char szIpv6[64];            // ipv6
    char szCmd[513];            //手机侧命令字
} ST_QueryCond;

// ctrltype: 1:限时控制 2：验证码控制
enum  CTRLTYPE_TYPE
{
    CTRLTYPE_LIMIT_TIME = 1,
    CTRLTYPE_VERIFY_CODE,
    CTRLTYPE_QUESTION,
};

// 打击信息
typedef  struct __tagBeatInfo
{
    unsigned int uiCtrlType;            // 限时封号 或 弹验证码
    unsigned int uiBlockEndTime;        // 结束时间
    EN_DimensionId enQueryDimensionId;  // 打击信息维度
} ST_BeatInfo;

/***************************************************************
Description: 打击信息查询。 本库所有查询接口的通用入口。
Input:
    _pstQueryCond: 查询条件
Output:
    _pstBeatInfo: 打击信息
    _pszErrMsg: 查询出错时存放出错原因的缓冲
    _sErrMsgSize: 存放出错原因的缓冲大小
Return:
    返回值 = 0, 查询成功。无打击限制
    返回值 = 1, 查询成功。有打击限制，结果保存在 _pstBeatInfo 中
    返回值 < 0, 查询失败。失败原因保存在 _pszErrMsg 中
***************************************************************/
int sac_query_beatinfo(ST_BeatInfo * _pstBeatInfo,
                       char * _pszErrMsg,
                       size_t _sErrMsgSize,
                       const ST_QueryCond * _pstQueryCond);

/**********  如下两个命名空间的内容为老版本接口  **************/

namespace sac_agent
{
#define SAC_QUERY_LIB_INIT_SUCC         0
#define SAC_QUERY_NONE_LIMIT            0
#define SAC_QUERY_FOUND                 (-1)
#define SAC_QUERY_INIT_ERROR            (-2)
#define SAC_QUERY_ERROR                 (-3)

#define SAC_QUERY_MESSAGE_FORBIDDEN     (-4)
#define SAC_QUERY_MESSAGE_AUTH_CODE     (-5)
#define SAC_QUERY_ACTION_FORBIDDEN      (-6)
#define SAC_QUERY_ACTION_AUTH_CODE      (-7)


#define SAC_BAD_QUERY_TIMEOUT           (-100)
#define SAC_QUERY_INIT_CONF_ERROR       (-101)
#define SAC_QUERY_PARAM_INVALID         (-102)


enum FLAG_CODE
{
    // 验证码限制标志位，游戏动作时需要验证码验证，针对不同纬度openid,ip,uin,UID进行限制
    FLAG_ACTION_AUTH_CODE_OPENID = 100,
    FLAG_ACTION_AUTH_CODE_UIN,
    FLAG_ACTION_AUTH_CODE_UID_8BYTE,
    FLAG_ACTION_AUTH_CODE_IP,

    // 动作限制标志位，对外挂黑名单用户封号，所有游戏动作禁止
    FLAG_ACTION_FORBIDDEN_OPENID = 200,
    FLAG_ACTION_FORBIDDEN_UIN,
    FLAG_ACTION_FORBIDDEN_UID_8BYTE,
    FLAG_ACTION_FORBIDDEN_IP,

    // 外挂警告标识位，
    FLAG_WG_WARN_OPENID = 300,
    FLAG_WG_WARN_UIN,
    FLAG_WG_WARN_UID_8BYTE,

    // 禁止发表言论标志位，不允许进行言论
    FLAG_MESSAGE_FORBIDDEN_OPENID = 400,
    FLAG_MESSAGE_FORBIDDEN_UIN,
    FLAG_MESSAGE_FORBIDDEN_UID_8BYTE,
    FLAG_MESSAGE_FORBIDDEN_IP,

    // 言论时需要验证码验证，
    FLAG_MESSAGE_AUTH_CODE_OPENID = 500,
    FLAG_MESSAGE_AUTH_CODE_UIN,
    FLAG_MESSAGE_AUTH_CODE_UID_8BYTE,
    FLAG_MESSAGE_AUTH_CODE_IP,

    // 禁止注册标识位，
    FLAG_REGISTER_FORBIDDEN_OPENID = 600,
    FLAG_REGISTER_FORBIDDEN_UIN,
    FLAG_REGISTER_FORBIDDEN_UID_8BYTE,
    FLAG_REGISTER_FORBIDDEN_IP
};

typedef struct _OpenID
{
    unsigned char  openid[16];
    /*************************************************
      Description:    32字节可打印字符串OpenID，转换为16字节二进制OpenID
      Input:
                szID        32字节可打印字符串OpenID，

      Return:
                0            成功
                -1           失败

      Others:
    *************************************************/
    int SetValue(char* szID);
}OpenID;

/*************************************************
  Description:    agent单个限制标志位查询接口
  Input:
            pID           16字节OPENID指针，
            uiAppid       应用appid
            uiFlag        安全限制功能标志位，每个限制标志代表的意义请参照枚举FLAG_CODE的解释
                          具体取值范围如下：
                          FLAG_MESSAGE_FORBIDDEN_OPENID
                          FLAG_MESSAGE_AUTH_CODE_OPENID
                          FLAG_ACTION_FORBIDDEN_OPENID
                          FLAG_ACTION_AUTH_CODE_OPENID

  Output:
            pEndtime    若限制标志位有效，该时间指针返回限制标志位到期时间
            szMsg       调用方传入字符串会带出相应的接口调用日志
            iMsglen     调用方传入字符串的最大长度

  Return:
            SAC_QUERY_NONE_LIMIT    限制功能标志位无效
            SAC_QUERY_FOUND         查询的限制功能标志位生效
            SAC_QUERY_INIT_ERROR    查询过程初始化失败
            SAC_QUERY_ERROR         查询过程失败

  Others:
*************************************************/
int sac_agent_query(OpenID* pID, unsigned int uiAppid,
                    unsigned int uiFlag, unsigned int* pEndtime, char* szMsg, int iMsglen);


/*************************************************
  Description:    agent单个限制标志位查询接口
  Input:
            uiID        4字节ID，取值为 UIN 或 IP(主机字节序), 与uiFlag的取值对应
            uiAppid     应用appid
            uiFlag      安全限制功能标志位，每个限制标志代表的意义请参照枚举FLAG_CODE的解释
                        具体取值范围如下：
                        XXXX_FORBIDDEN_UIN
                        XXXX_AUTH_CODE_UIN
                        XXXX_FORBIDDEN_IP
                        XXXX_AUTH_CODE_IP

  Output:
            pEndtime    若限制标志位有效，该时间指针返回限制标志位到期时间
            szMsg       调用方传入字符串会带出相应的接口调用日志
            iMsglen     调用方传入字符串的最大长度

  Return:
            SAC_QUERY_NONE_LIMIT    限制功能标志位无效
            SAC_QUERY_FOUND         查询的限制功能标志位生效
            SAC_QUERY_INIT_ERROR    查询过程初始化失败
            SAC_QUERY_ERROR         查询过程失败

  Others:
*************************************************/
int sac_agent_query(unsigned int uiID, unsigned int uiAppid,
                    unsigned int uiFlag, unsigned int* pEndtime, char* szMsg, int iMsglen);


/*************************************************
  Description:  agent文字信息安全控制标志查询接口
                该接口封装了OPENID，UIN，IP三个纬度的受限名单查询逻辑，受限逻辑包括了禁言和弹验证码
                若传入参数的某一个纬度受限，则会返回相应的受限结果
  Input:
            pID         16字节OPENID，若不查询受限OPENID，则指针传为空指针
            uiUin       4字节用户UIN(QQ号码)，若不查询受限UIN，则参数填0
            uiIp        4字节无符号整型IP值，若不查询受限IP，则参数填0
            uiAppid     应用appid

  Output:
            pEndtime    若限制标志位有效，该时间指针返回限制标志位到期时间
            szMsg       调用方传入字符串会打印相应的接口调用日志
            iMsglen     调用方传入字符串的最大长度

  Return:
            SAC_QUERY_NONE_LIMIT        查询名单不在任何限制名单里面，不需要进行限制
            SAC_QUERY_INIT_ERROR        查询过程初始化失败
            SAC_QUERY_ERROR             查询过程错误
            SAC_QUERY_MESSAGE_FORBIDDEN        查询名单在禁言名单里，该文字信息禁止发送（禁言）
            SAC_QUERY_MESSAGE_AUTH_CODE        查询名单在验证码限制名单里，文字信息发送需要通过验证码验证

  Others:
*************************************************/
int sac_agent_message_security(OpenID* pID, unsigned int uiUin, unsigned int uiIp,
                               unsigned int uiAppid, unsigned int* pEndtime, char* szMsg, int iMsglen);


/*************************************************
  Description:  游戏安全访问控制标志位查询接口
                该接口封装了OPENID，UIN，IP三个纬度的游戏访问安全受限名单查询逻辑，受限逻辑包括了外挂封号和弹验证码
                若传入参数的某一个纬度受限，则会返回相应的受限结果
  Input:
            pID         16字节OPENID，若不查询受限OPENID，则指针传为空指针
            uiUin       4字节用户UIN(QQ号码)，若不查询受限UIN，则参数填0
            uiIp        4字节无符号整型IP值，若不查询受限IP，则参数填0
            uiAppid     应用appid

  Output:
            pEndtime    若限制标志位有效，该时间指针返回限制标志位到期时间
            szMsg       调用方传入字符串会打印相应的接口调用日志
            iMsglen     调用方传入字符串的最大长度

  Return:
            SAC_QUERY_NONE_LIMIT        查询名单不在任何限制名单里面，正常操作
            SAC_QUERY_INIT_ERROR        查询过程初始化失败
            SAC_QUERY_ERROR             查询过程错误
            SAC_QUERY_ACTION_FORBIDDEN        查询名单在游戏动作受限名单里，不允许正常进行游戏（封号）
            SAC_QUERY_ACTION_AUTH_CODE        查询名单在验证码限制名单里，游戏动作需要通过验证码验证

  Others:
*************************************************/
int sac_agent_action_security(OpenID* pID, unsigned int uiUin, unsigned int uiIp,
                              unsigned int uiAppid, unsigned int* pEndtime, char* szMsg, int iMsglen);

}

namespace SAC_AGENT_API
{

/* 查询结果*/
enum EN_SACCheckResult
{
    SAC_CHECK_PASS = 0,     // 通过
    SAC_CHECK_IP_CODE,      // 恶意IP , 弹验证码
    SAC_CHECK_IP_DENY,      // 受限IP
    SAC_CHECK_PUIN_CODE,    // 恶意PostUin, 弹验证码
    SAC_CHECK_PUIN_DENY,    // 受限PostUin
    SAC_CHECK_HUIN_CODE,    // 恶意HostUin, 弹验证码
    SAC_CHECK_HUIN_DENY     // 受限HostUin
};

enum EN_CheckFlag
{
    CHKFLG_CIP_ONLY = 1,                // 只校验客户端IP
    CHKFLG_POSTUIN_ONLY,                // 只校验发送UIN
    CHKFLG_HOSTUIN_ONLY,                // 只校验接收UIN
    CHKFLG_IP_POSTUIN_OR,               // 校验CIP 和PostUin 或关系
    CHKFLG_IP_HOSTUIN_OR,               // 校验CIP 和HostUin 或关系
    CHKFLG_POSTUIN_HOSTUIN_OR,          // 校验PostUin HostUin 或关系
    CHKFLG_IP_POSTUIN_HOSTUIN_OR        // 校验IP PostUin HostUin 或关系
};

/*返回给业务前端的恶意等级值*/
enum EN_AbnormalLevel
{
    LEVEL_0 = 0,
    LEVEL_1,
    LEVEL_2,
    LEVEL_3,
    LEVEL_4,
    LEVEL_5,
    LEVEL_6,
    LEVEL_7,
    LEVEL_8,
    LEVEL_9,
    LEVEL_10,
    LEVEL_11,
    LEVEL_12,
    LEVEL_13,
    LEVEL_14,
    LEVEL_15,
    LEVEL_16,
    LEVEL_17,
    LEVEL_18,
    LEVEL_19,
    LEVEL_20,
};

/**********************************************************************
Function    : qzone_sac_api_check
Description : 安全旁路查询接口
Input       : _uiClientIP     -> Qzone 留言客户端IP。主机字节序
              _uiPostUin      -> 发送留言QQ 号码
              _uiHostUin      -> 接收留言QQ 号码
              _iErrMsgLen     -> 错误提示消息BUF长度
              _iFlag          -> 校验类型选择, 取值范围 EN_CheckFlag
Output      : _puiResult      -> 返回检测结果, 取值范围 EN_SACCheckResult
              _puiLevel       -> 返回恶意度等级, 取值范围 EN_AbnormalLevel
              _pszErrMsg      -> 返回错误信息
Return      : 0               -> OK
            : 其他            -> Failed
Calls       :
Called By   : 无
Others      : *_puiResult = SAC_CHECK_PASS; 表示通过
              *_puiResult = SAC_CHECK_IP_CODE; 表示恶意ClientIP ，弹验证码
              *_puiResult = SAC_CHECK_IP_DENY; 表示恶意ClientIP ，前端限制
              *_puiResult = SAC_CHECK_PUIN_CODE; 表示恶意PostUin，弹验证码
              *_puiResult = SAC_CHECK_PUIN_DENY; 表示恶意PostUin，前端限制
              *_puiResult = SAC_CHECK_HUIN_CODE; 表示恶意HostUin，弹验证码
              *_puiResult = SAC_CHECK_HUIN_DENY;表示恶意HostUin，前端限制
              *_puiLevel 表示恶意等级值, 最高为20 级
**********************************************************************/
int qzone_sac_api_check(unsigned int _uiClientIP,
                        unsigned int _uiPostUin,
                        unsigned int _uiHostUin,
                        int * _puiResult, int * _puiLevel, unsigned int * _puiEndTime,
                        char * _pszErrMsg, int _iErrMsgLen,
                        int _iFlag = CHKFLG_IP_POSTUIN_OR);


/**********************************************************************
Function    : qzone_sac_api_check
Description : 安全旁路查询接口
Input       : _uiAppid        -> 应用appid
              _uiClientIP     -> Qzone 留言客户端IP。主机字节序
              _uiPostUin      -> 发送留言QQ 号码
              _uiHostUin      -> 接收留言QQ 号码
              _iErrMsgLen     -> 错误提示消息BUF长度
              _iFlag          -> 校验类型选择, 取值范围 EN_CheckFlag
Output      : _puiResult      -> 返回检测结果, 取值范围 EN_SACCheckResult
              _puiLevel       -> 返回恶意度等级, 取值范围 EN_AbnormalLevel
              _pszErrMsg      -> 返回错误信息
Return      : 0               -> OK
            : 其他            -> Failed
Calls       :
Called By   : 无
Others      : *_puiResult = SAC_CHECK_PASS; 表示通过
              *_puiResult = SAC_CHECK_IP_CODE; 表示恶意ClientIP ，弹验证码
              *_puiResult = SAC_CHECK_IP_DENY; 表示恶意ClientIP ，前端限制
              *_puiResult = SAC_CHECK_PUIN_CODE; 表示恶意PostUin，弹验证码
              *_puiResult = SAC_CHECK_PUIN_DENY; 表示恶意PostUin，前端限制
              *_puiResult = SAC_CHECK_HUIN_CODE; 表示恶意HostUin，弹验证码
              *_puiResult = SAC_CHECK_HUIN_DENY;表示恶意HostUin，前端限制
              *_puiLevel 表示恶意等级值, 最高为20 级
**********************************************************************/
int qzone_sac_api_check(unsigned int _uiAppid,
                        unsigned int _uiClientIP,
                        unsigned int _uiPostUin,
                        unsigned int _uiHostUin,
                        int * _puiResult, int * _puiLevel, unsigned int * _puiEndTime,
                        char * _pszErrMsg, int _iErrMsgLen,
                        int _iFlag = CHKFLG_IP_POSTUIN_OR);
}

#endif  // __sac__agent__h__
