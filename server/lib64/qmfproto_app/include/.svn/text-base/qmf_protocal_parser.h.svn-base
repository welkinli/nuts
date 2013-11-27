/*
 * qmf_protocal_parser.h
 *
 *  Created on: 2012-4-19
 *      Author: himzheng
 */

#ifndef QMF_PROTOCAL_PARSER_H_
#define QMF_PROTOCAL_PARSER_H_

#include "qmf_protocal_define.h"
#include "pdu_header.h"

#include <arpa/inet.h>

namespace QMF_PROTOCAL
{

#define IN
#define OUT

    enum QmfQzoneProtocalCmd
    {
        QMF_PDU_CMD_UPSTREAM = 0x01,//使用QmfUpstream结构体
        QMF_PDU_CMD_DOWNSTREAM = 0x02,//使用QmfDownstream结构体
        QMF_PDU_CMD_PROVIDER = 0x03,//使用QmfProvidorReq结构体
        QMF_PDU_CMD_NOBODY = 0x100,//没有body 只是一个qzone头
    };

class QmfProtocalParser
{
    public:

        /**
         * 功能: 解析从QMF传过来的协议，提供解释head头数据流和busi数据流
         * 参数说明:
         *      输入
         *          reqbuff: qmf端过来的数据流
         *          reqbufflen: qmf端过来的数据流长度
         *      输出
         *          stQmfUpstream: QmfUpstream结构体，包含本次通讯的相关信息，业务数据stQmfUpstream.BusiBuff
         * 返回值:
         *   <-100: jce编码失败
         *     -11: busibufflen小于业务数据包大小
         *      =0: 解码成功
         */
        static int DecodeRequest(IN const char* reqbuff, IN int reqbufflen, OUT QMF_PROTOCAL::QmfUpstream &stQmfUpstream)
        {
            struct pdu_protocol_header _header;
            int ret = DecodeRequest(reqbuff, reqbufflen, stQmfUpstream, _header);
            return ret < 0 ? ret : 0;
        }

        /**
         * 功能: 解析从QMF传过来的协议，提供解释head头数据流和busi数据流
         * 参数说明:
         *      输入
         *          reqbuff: qmf端过来的数据流
         *          reqbufflen: qmf端过来的数据流长度
         *          busibuff: 业务数据包的数据流  ，用于业务处理
         *      输出
         *          busibufflen: 业务数据包长度，输入时请sizeof(busibuff)，返回时为业务数据流长度
         * 返回值:
         *   <-100: jce编码失败
         *     -11: busibufflen小于业务数据包大小
         *     >=0: 解码成功，返回业务数据流长度
         */
        static int DecodeRequest(IN const char* reqbuff, IN int reqbufflen, OUT char* busibuff, OUT int& busibufflen)
        {
            QmfUpstream _qmfup;
            int ret = DecodeRequest(reqbuff, reqbufflen, _qmfup);
            if (ret < 0)
            {
                return ret;
            }
            if (_qmfup.BusiBuff.size() == 0)
            {
                busibufflen = 0;
                return 0;
            }

            if ((unsigned)busibufflen < _qmfup.BusiBuff.size())
            {
                return -11;
            }
            memcpy(busibuff, &_qmfup.BusiBuff[0], _qmfup.BusiBuff.size());
            busibufflen = _qmfup.BusiBuff.size();

            return busibufflen;
        }

        /**
         * 功能: 编码QMF回包协议
         * 参数说明:
         *      输入
         *          bizcode: 业务返回码
         *          reqbuff: qmf端过来的数据流
         *          reqbufflen: qmf端过来的数据流长度
         *          busibuff: 返回给客户端的数据流
         *          busibufflen: 返回给客户端的数据流长度
         *      输出
         *          respbuff: 返回给qmf端的数据流
         *          respbufflen: 返回给qmf端的数据流长度 ，输入时先sizeof(respbuff)
         * 返回值:
         *   <-100: jce编码失败
         *       0: 编码成功
         */
        static int EncodeResponse(IN short bizcode,
                                  IN const char* reqbuff, IN int reqbufflen,
                                  IN const char* busibuff, IN int busibufflen,
                                  OUT char* respbuff, OUT int& respbufflen)
        {
            QmfUpstream _qmfup;
            struct pdu_protocol_header _header;
            int ret = DecodeRequest(reqbuff, reqbufflen, _qmfup, _header);
            if (ret < 0)
            {
                return ret;
            }
            _header.cmd = QMF_PDU_CMD_DOWNSTREAM;
            QmfDownstream _qmfdown;
            _qmfdown.Seq       = _qmfup.Seq;
            _qmfdown.Uin         = _qmfup.Uin;
            _qmfdown.WnsCode     = 0;
            _qmfdown.BizCode     = bizcode;
            _qmfdown.ServiceCmd  = _qmfup.ServiceCmd;
            _qmfdown.BusiBuff.assign(busibuff, busibuff + busibufflen);
            ret = struct_QmfDownstream_pack(
                    &_qmfdown, (uint8_t *)respbuff, &respbufflen, &_header);

            return ret;
        }

        /**
         * 功能: 判断数据包是否接收完整
         * 参数说明:
         *      输入
         *          reqbuff: qmf端过来的数据流
         *          reqbufflen: qmf端过来的数据量长度
         *      输出
         *          datalen: 检测到完整qmf包的长度
         * 返回值:
         *      <0: -1为包头SOH标识错误 -2数据包长度错误
         *       0: 继续收包
         *      >0: 收到完整的qmf包
         */
        static int IsPktComplete(IN const char* reqbuff, IN int reqbufflen, OUT int& datalen)
        {
            if (reqbuff[0] != ISDProtocolSOH)
            {
                return -1;
            }
            const int _head_len = (int)(sizeof(char) + sizeof(struct pdu_protocol_header));
            if (reqbufflen < _head_len)
            {
                return 0;
            }

            struct pdu_protocol_header* pPacket = (struct pdu_protocol_header*)(reqbuff+1);

            int _pack_len = ntohl(pPacket->len);
            if (_pack_len < _head_len)
            {
                return -2;
            }

            if(reqbufflen < _pack_len)
            {
                return 0;
            }

            return (datalen = _pack_len);
        }

	static int IsPktComplete_feedback(IN const char* reqbuff, IN int reqbufflen, OUT int& datalen, OUT int &next_need)
        {
            if (reqbuff[0] != ISDProtocolSOH)
            {
                return -1;
            }
            const int _head_len = (int)(sizeof(char) + sizeof(struct pdu_protocol_header));
            if (reqbufflen < _head_len)
            {
            	next_need = _head_len - reqbufflen;
                return 0;
            }

            struct pdu_protocol_header* pPacket = (struct pdu_protocol_header*)(reqbuff+1);

            int _pack_len = ntohl(pPacket->len);
            if (_pack_len <= _head_len)
            {
                return -2;
            }

            if(reqbufflen < _pack_len)
            {
            	next_need = _pack_len - reqbufflen;
                return 0;
            }

		next_need = 0;
            return (datalen = _pack_len);
        }


    private:
        QmfProtocalParser();
        ~QmfProtocalParser();

        static int DecodeRequest(const char* reqbuff, int reqbufflen,
                                 QMF_PROTOCAL::QmfUpstream &stQmfUpstream, pdu_protocol_header &header)
        {
            int ret = struct_QmfUpstream_unpack(
                    (uint8_t *)reqbuff, &reqbufflen, &stQmfUpstream, &header);
            if (ret != 0)
            {
                return ret;
            }
            return stQmfUpstream.BusiBuff.size();
        }

};

} // namespace QMF_PROTOCAL

#endif /* QMF_PROTOCAL_PARSER_H_ */
