/*
 * CQmfCryptor.cpp
 *
 *  Created on: 2012-5-10
 *      Author: himzheng
 */

#include "CQmfCryptor.h"
#include "vas_key.h"

namespace QMF
{

#define DECRYPT_SIG_TIMEOUT     ( 2)//虽然签名解密成功但是签名超期
#define DECRYPT_SUCC            ( 1)//签名解密成功，且签名在有效期内
#define DECRYPT_FAIL            ( 0)//签名解密失败
#define DECRYPT_FAILD_NO_KEY    (-1)//没有vaskey，非常严重的错误，强烈建议在程序中增强对这个错误的检测，如果出现这个错误，建议报警。一般在调试时，经常是由于没有安装vaskey agent或者申请服务器权限，需要按照文档说明（一）（二）两部分的内容安装agent和申请服务器权限。
#define DECRYPT_FAILD_VAS_ERR   (-2)//传入的vasid错误
#define DECRYPT_FAILD_PARA_ERR  (-3)//传入的参数错误

#define KEY_FOR_QZONE33     "Aakkf181u02MKwV1"

    void outputf(const char *buf,unsigned int len,unsigned int line) {
        for ( unsigned int i=0;i<(len-1)/line+1;++i ) {
            for ( unsigned int j=0;j<line;++j )
            {
                if ( j+i*line<len ) {
                unsigned char p = (unsigned char)buf[i*line+j];
                    printf("%02x ",p);
                } else printf("   ");
            }
            printf("| ");
            for ( unsigned int j=0;j<line && j+i*line<len;++j ) {
                unsigned char p = (unsigned char)buf[i*line+j];
                if ( p>=0x20 && p<=0x7F ) printf("%c",p);
                else printf(".");
            }
            printf("\n");
        }
        if ( len % line )
            printf("\n");
    }

    CQmfCryptor::CQmfCryptor()
    {
        // TODO Auto-generated constructor stub

    }

    CQmfCryptor::~CQmfCryptor()
    {
        // TODO Auto-generated destructor stub
    }


    int CQmfCryptor::encrypt(const char* input, int inputlen, QMF_PROTOCAL::QmfHead &head,
                 char* output, int* outputlen)
    {/*
        if (*outputlen < inputlen) {
            return -2;
        }
        int ret = 0;
        if (head.Enc > 0)//加密
        {
            char decryptkey[16];
            if (head.Enc == 1) {//GTKEY_B2
                DecoGTKey_B2(head.B2.data(), head.B2.size(), head.Uin, head.Appid, decryptkey);
            }
            else if (head.Enc == 2) {//0000000000000000
                memset(decryptkey, 0, sizeof(decryptkey));
            }
            else if (head.Enc == 3) {//GTKEY_ST
                ret = DecoGTKey_ST(head.B2.data(), head.B2.size(), head.Uin, head.Appid, decryptkey);
                if (ret < 0) return ret;
            }
            else if (head.Enc == 11) {//For Qzone 3.3
                memcpy(decryptkey, KEY_FOR_QZONE33, 16);
            }
            else {
                return -1;
            }
            oi_symmetry_encrypt2(input, inputlen, decryptkey, output, outputlen);
        } else {//不加密
            *outputlen = inputlen;
            memcpy(output, input, inputlen);
        }*/
        return 0;
    }

    int CQmfCryptor::decrypt(QMF_PROTOCAL::QmfHead &head,
                 char* output, int* outputlen)
    {	/*
        if ((unsigned)*outputlen < head.BodyLen) {
            return -2;
        }
        int ret = 0;
        if (head.Enc > 0)//加密
        {
            char decryptkey[16];
            if (head.Enc == 1) {//GTKEY_B2
                DecoGTKey_B2(head.B2.data(), head.B2.size(), head.Uin, head.Appid, decryptkey);
            }
            else if (head.Enc == 2) {//0000000000000000
                memset(decryptkey, 0, sizeof(decryptkey));
            }
            else if (head.Enc == 3) {//GTKEY_ST
                ret = DecoGTKey_ST(head.B2.data(), head.B2.size(), head.Uin, head.Appid, decryptkey);
                if (ret < 0) return ret;
            }
            else if (head.Enc == 11) {//For Qzone 3.3
                memcpy(decryptkey, KEY_FOR_QZONE33, 16);
            }
            else {
                return -1;
            }
            oi_symmetry_decrypt2(
                    (char*)head.Body, head.BodyLen, decryptkey, output, outputlen);
        } else {//不加密
            *outputlen = head.BodyLen;
            memcpy(output, head.Body, head.BodyLen);
        }*/
        return 0;
    }

    int CQmfCryptor::DecoGTKey_ST(const char* st, size_t stlen, unsigned uin, int appid, char* gtkey_st)
    {
        /**
         * ST = wStVer(1)+dwTime+wNextFieldLen+
         *      KS_ST (dwRandom+wStVer(1)+dwUin+dwTime+dwSSOVer+dwAppID+dwAppClientVer+dwClientIP+GTKey_ST+wOtherData+stOtherData+wNextFieldLen+stCustomInfo_ST[+ wNextFieldLen + stMibaoInfo (SSOVer>=0x310)]))
         */
        time_t _now;
        time(&_now);
        char outbuf[1024];
        int outlen = sizeof(outbuf);
        int ret = KeyAuthSys_TeaDecrypt(//请使用这个接口函数
                st + 8, stlen - 8,
                "0000000000000000",
                207, uin, _now,
                "",//客户端的IP
                1,//客户端的版本
                -1,1,
                outbuf, &outlen);
        if (ret != DECRYPT_SUCC)
        {
            /// 减去100，避免使用0当做失败的情况
            return ret - 100;
        }

        //TODO 取出时间看看有木有过期 需要完善加强数据的校验
        uint32_t _appid = ntohl(*(int*)(outbuf+18));
        if (appid != _appid)
        {
//            error_log("TeaDecrypt error. appid fail, de-app=%u qmf-app=%u", appid, head.Appid);
        }
        memcpy(gtkey_st, outbuf+30, 16);//接出GTKey_ST
        return 0;
    }

    int CQmfCryptor::DecoGTKey_B2(const char* b2, size_t b2len, unsigned uin, int appid, char* gtkey_b2)
    {
        time_t _now;
        time(&_now);
        char outbuf[1024];
        int outlen = sizeof(outbuf);

        oi_symmetry_decrypt2(
                b2, b2len, GetGenerateKey(appid), outbuf, &outlen);

        //TODO 验证appid time
        memcpy(gtkey_b2, outbuf+12, 16);//接出GTKey_ST
        return 0;
    }

    int CQmfCryptor::GenerateB2(int appid, unsigned uin, char* output, int* outputlen, char* gtkey_b2)
    {
        /**
         * B2 = dwUin+dwTime+dwAppid+GTKey_B2
         */
        for (int i=0; i<16; i++)
        {
            gtkey_b2[i] = rand()%255;
        }
        time_t _now;
        time(&_now);
	int now_tmp = _now & 0xffffffff;
        char b2buff[28];
	memcpy(b2buff, &uin, sizeof(uin));
	memcpy(b2buff + 4, &now_tmp, sizeof(now_tmp));
	memcpy(b2buff + 8, &appid, sizeof(appid));
        memcpy(b2buff+12, gtkey_b2, 16);

        oi_symmetry_encrypt2(b2buff, sizeof(b2buff), GetGenerateKey(appid), output, outputlen);

        return 0;
    }

    char* CQmfCryptor::GetGenerateKey(int appid)
    {
        static char decryptkey[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        return decryptkey;
    }

}
