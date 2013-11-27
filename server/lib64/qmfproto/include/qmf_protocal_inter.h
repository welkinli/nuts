/*
 * qmf_protocal_inter.h
 *
 *  Created on: 2012-4-19
 *      Author: himzheng
 */

#ifndef QMF_PROTOCAL_INTER_H_
#define QMF_PROTOCAL_INTER_H_
#include <string>
#include "wup.h"

#define QMF_HEAD_STX 0x02
#define QMF_ACC_HEAD_SOH 0x01

#define	WNS_SIGN_STR	"wns\0"
#define	WNS_SIGN_INT	0x736e77

namespace QMF_PROTOCAL
{
    using namespace std;

    struct QmfHead
    {
            //uint8_t     STX;//起始标识符 固定为0x02
            uint32_t Wns_sign;
            uint32_t    Len;//包体总长度
            uint8_t     Ver;//版本
            uint8_t     Enc;//是否加密
            uint32_t    Flag;//控制位
            uint32_t    Appid;
            uint64_t    Uin;//uin
            string      B2;//QMF私有的Token，类似ST
            uint32_t    ComprLen;//ver>=2，启动压缩时，表示原数据大小
            uint32_t seq;//ver>=3
			/////下面两个是Decode后自动生成的，并非QmfHead本身，为了方便获取到body部分
            uint32_t    HeadLen;//头部长度，decode时计算出来
            uint32_t    BodyLen;//body长度，decode时计算出来
            uint8_t     *Body;//body起始，decode时赋值

            void init()
            {
                //STX = QMF_HEAD_STX;
                Wns_sign = WNS_SIGN_INT;
                Len = 0;
                Ver = 1;
                Enc = 0;
                Appid = 0;
                Uin = 0;
                B2.clear();
                HeadLen = 0;
                BodyLen = 0;
                Body = NULL;
                Flag = 0;
                ComprLen = 0;
		seq = 0;
            }
    };

#pragma pack(1)
    struct QmfAccHead
    {
            uint8_t        SOH;//起始标识符 固定为0x01
            uint32_t       Len;//包体总长度
            uint8_t        Ver;//acc版本
            uint32_t       Seq;//序列号
            uint32_t       ClientIP;//客户端IP
            uint16_t       ClientPort;//端口
            uint64_t       ClientID;//链接ID
            uint32_t       AccIp;//accip
            uint16_t       AccPort;//accport
            /* Flag的最低位保留为acc通知push是否成功，次低位为是否有akamai标志*/
            uint8_t        Flag;//控制位

            /////////////以下为ver2
            uint64_t       Ticks;//时间戳
            
            uint32_t	AccIp_wan;
	    uint16_t	service_port;
	    uint8_t reserved[10];
		
            void init()
            {
                SOH = QMF_ACC_HEAD_SOH;
                Len = 0;
                Ver = 1;
                ClientIP = 0;
                ClientPort = 0;
                ClientID = 0;
                AccIp = 0;
                AccPort = 0;
                Seq = 0;
                Flag = 0;
                Ticks = 0;
            }
    };
#pragma pack()

} // namespace QMF_PROTOCAL

#endif /* QMF_PROTOCAL_INTER_H_ */
