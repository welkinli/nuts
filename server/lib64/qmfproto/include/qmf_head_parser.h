/*
 * qmf_head_parser.h
 *
 *  Created on: 2012-5-7
 *      Author: himzheng
 */

#ifndef QMF_HEAD_PARSER_H_
#define QMF_HEAD_PARSER_H_

#include "qmf_protocal_inter.h"

namespace QMF_PROTOCAL
{

#define PT_SWAP_DDWORD(x)  \
    ((((x) & 0xff00000000000000llu) >> 56) | \
     (((x) & 0x00ff000000000000llu) >> 40) | \
     (((x) & 0x0000ff0000000000llu) >> 24) | \
     (((x) & 0x0000ff0000000000llu) >> 24) | \
     (((x) & 0x000000ff00000000llu) >> 8) | \
     (((x) & 0x00000000ff000000llu) << 8) | \
     (((x) & 0x0000000000ff0000llu) << 24) | \
     (((x) & 0x000000000000ff00llu) << 40) | \
     (((x) & 0x00000000000000ffllu) << 56) )

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PT_HTONLL(x) PT_SWAP_DDWORD(x)
#else
#define PT_HTONLL(x) (x)
#endif

#ifndef htonll
#define htonll(x) PT_HTONLL(x)
#endif

#ifndef ntohll
#define ntohll(x) htonll(x)
#endif

    enum qmf_head_error{
        err_qmf_head_illegal = -1,
        err_qmf_head_stx = -2,
        err_qmf_head_b2len = -3,
        err_qmf_head_headlen_illegal = -4,

        err_qmf_acc_illegal = -101,
        err_qmf_acc_not_enough = -102,
    };
    class QmfHeadParser
    {
        public:
            QmfHeadParser() {};
            virtual ~QmfHeadParser() {};

            int DecodeQmfHead(const char* reqbuff, unsigned reqbufflen, QMF_PROTOCAL::QmfHead &head)
            {
            	uint32_t *sign_ptr = NULL;
			
                if (reqbufflen < 39) return err_qmf_head_illegal;

                sign_ptr = (uint32_t *)reqbuff;
                if (*sign_ptr != WNS_SIGN_INT) return err_qmf_head_stx;
                
                head.Wns_sign = WNS_SIGN_INT;

                uint32_t pos = 4;
                head.Len = ntohl(*((uint32_t*)(reqbuff + pos)));
                pos += 4;

                head.Ver = reqbuff[pos++];

                //head.Enc = reqbuff[pos++];

                //head.Flag = ntohl(*(uint32_t*)(reqbuff + pos));
                //pos += 4;

                head.Cmd = ntohl(*(uint32_t*) (reqbuff + pos));
                pos += 4;

                head.Uin = ntohll(*(uint64_t*) (reqbuff + pos));
                pos += 8;

                head.Key = ntohll(*(uint64_t*) (reqbuff + pos));
                pos += 8;

                head.seq = ntohl(*(uint32_t*) (reqbuff + pos));
                pos += 4;

                head.ReserveFlag = ntohl(*(uint32_t*) (reqbuff + pos));
                pos += 2;

               //uint16_t b2len = ntohs(*(uint16_t*) (reqbuff + pos));
                //pos += 2;

//                if (b2len + pos > reqbufflen) return err_qmf_head_b2len;
//                if (b2len > 0) {
//                    head.B2.assign(reqbuff + pos, b2len);
//                } else {
//                    head.B2.clear();
//                }
//                pos += b2len;

//                // ver=2 第二版本新增字段
//                if (head.Ver >= 2)
//                {
//                    head.ComprLen = ntohl(*(uint32_t*) (reqbuff + pos));
//                    pos += 4;
//
//                    if (head.Ver >= 3)
//                    {
//                        head.seq = ntohl(*(uint32_t*) (reqbuff + pos));
//                        pos += 4;
//                    }
//                }

                head.HeadLen = pos;
                head.BodyLen = head.Len - pos;
                head.Body = (uint8_t*)(reqbuff + pos);
                return 0;
            }

            int EncodeQmfHead(char* outbuff, int* outlen,
                                     QMF_PROTOCAL::QmfHead &head,
                                     const char* busibuff, int busilen)
            {
                if (*outlen < 28) return err_qmf_head_illegal;
                head.HeadLen = GetQmfHeadLen(head);
                if (head.HeadLen <= 0) return err_qmf_head_headlen_illegal;

                int pos = 0;
                //begin
                *(uint32_t*)(outbuff) = WNS_SIGN_INT;
                pos += 4;
                //len
                *(uint32_t*)(outbuff + pos) = htonl(head.HeadLen + busilen);
                pos += 4;
                //qmfver
                outbuff[pos++] = head.Ver;
                //enc
                //outbuff[pos++] = head.Enc;
                //flag
//                *(uint32_t*)(outbuff + pos) = htonl(head.Flag);
//                pos += 4;
                //appid
                *(uint32_t*)(outbuff + pos) = htonl(head.Cmd);
                pos += 4;
                //uin
                *(uint64_t*)(outbuff + pos) = htonll(head.Uin);
                pos += 8;
                //Key
                *(uint64_t*)(outbuff + pos) = htonll(head.Key);
                pos += 8;
                *(uint64_t*)(outbuff + pos) = htonll(head.seq);
                pos += 4;
                *(uint64_t*)(outbuff + pos) = htonll(head.ReserveFlag);
                pos += 2;
//                //b2len
//                *(uint16_t*)(outbuff + pos) = htons(head.B2.size());
//                pos += 2;
//                if (head.B2.size() > 0) {
//                    //b2
//                    memcpy(outbuff+pos, head.B2.c_str(), head.B2.size());
//                    pos += head.B2.size();
//                }
//                //ver>=2 comprlen
//                if (head.Ver >= 2)
//                {
//                    *(uint32_t*) (outbuff + pos) = htonl(head.ComprLen);
//                    pos += 4;
//
//                    if (head.Ver >= 3)
//                    {
//                        *(uint32_t*) (outbuff + pos) = htonl(head.seq);
//                        pos += 4;
//                    }
//                }
                head.HeadLen = pos;
                head.BodyLen = busilen;
                if (busibuff) memcpy(outbuff+pos, busibuff, busilen);
                head.Body = (uint8_t *)(outbuff+pos);

                *outlen = pos + busilen;
                return 0;
            }

            /**
             * 获取QmfHead本身长度
             * @param head
             * @return
             */
            int GetQmfHeadLen(QMF_PROTOCAL::QmfHead &head)
            {
            	return 39;
            	/*int len = 0;
				
                if (head.Ver >= 2)
                {
                    //wns_sign+len+ver+enc+flag+appid+uin+b2len+b2+compresslen
                    len = 4 + 4 + 1 + 1 + 4 + 4 + 8 + 2 + head.B2.size() + 4;

                    if (head.Ver >= 3)
                    {
                        len += 4; // seq
                    }

                    return len;
                }
				
                // ver=1 for default
                //wns_sign+len+ver+enc+flag+appid+uin+b2len+b2
                return 4+4+1+1+4+4+8+2+head.B2.size();
                */
            }


            int DecodeQmfAccHead(const char* buff, unsigned bufflen,
                                     QMF_PROTOCAL::QmfAccHead &head)
            {
                if (sizeof(QMF_PROTOCAL::QmfAccHead) > bufflen)
                {
                    return err_qmf_acc_illegal;
                }
                QMF_PROTOCAL::QmfAccHead *acchead = (QMF_PROTOCAL::QmfAccHead*)buff;
                head.Len        = ntohl(acchead->Len);
                head.Ver        = acchead->Ver;
                head.Seq        = ntohl(acchead->Seq);
                head.ClientIP   = ntohl(acchead->ClientIP);
                head.ClientPort = ntohs(acchead->ClientPort);
                head.ClientID   = ntohll(acchead->ClientID);
                head.AccIp      = ntohl(acchead->AccIp);
                head.AccPort    = ntohs(acchead->AccPort);
                head.ClientNum    = ntohs(acchead->ClientNum);
                for(int i=0;i<head.ClientNum;i++){
                	head.ClientUinList[i]=ntohl(acchead->ClientUinList[i]);
                }
                if(head.ClientNum <=1){
                	head.ClientUinList[0]=head.Uin;
                }
                head.AccPort    = ntohs(acchead->AccPort);
                head.Flag       = acchead->Flag;
                if (head.Ver >= 0x02) {
                    head.Ticks  = ntohll(acchead->Ticks);
                } else {
                    head.Ticks = 0;
                }
                if (head.Ver >= 0x03) {
                    head.AccIp_wan = ntohl(acchead->AccIp_wan);
                    head.service_port = ntohs(acchead->service_port);
                }
                return 0;
            }

            int EncodeQmfAccHead(QMF_PROTOCAL::QmfAccHead &head)
            {
                head.SOH        = QMF_ACC_HEAD_SOH;
               // head.Ver        = head.Ver;
                head.Seq        = htonl(head.Seq);
                head.Len        = htonl(head.Len);
                head.ClientIP   = htonl(head.ClientIP);
                head.ClientPort = htons(head.ClientPort);
                head.ClientID   = htonll(head.ClientID);
                head.AccIp      = htonl(head.AccIp);
                head.AccPort    = htons(head.AccPort);
              //  head.Flag       = head.Flag;
                head.Ticks      = htonll(head.Ticks);
                return 0;
            }

            /**
             * 获取AccHead长度
             * @param head
             * @return
             */
            int GetAccHeadLen(QMF_PROTOCAL::QmfAccHead &head)
            {
                    return (int)sizeof(QMF_PROTOCAL::QmfAccHead);
            }

            /**
             * 通过AccHead.Flag判断是否长链接
             * @param flag
             * @return
             */
            bool IsAccHeadFlagLongConn(int flag)
            {
                return (!(flag & 0x04));
            }

    };

}

#endif /* QMF_HEAD_PARSER_H_ */

