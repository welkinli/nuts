/*
 * CQmfCryptor.h
 *
 *  Created on: 2012-5-10
 *      Author: himzheng
 */

#ifndef CQMFCRYPTOR_H_
#define CQMFCRYPTOR_H_

#include "qmf_protocal_inter.h"
#include "qmf_protocal_define.h"
#include "oi_tea.h"

namespace QMF
{

    class CQmfCryptor
    {
        public:
            CQmfCryptor();
            virtual ~CQmfCryptor();

        public:
            int encrypt(const char* input, int inputlen, QMF_PROTOCAL::QmfHead &head,
                         char* output, int* outputlen);

            int decrypt(QMF_PROTOCAL::QmfHead &head, char* output, int* outputlen);
            /**
             * 生成B2密文
             */
            int GenerateB2(int appid, unsigned uin, char* output, int* outputlen, char* gtkey_b2);

            int DecoGTKey_ST(const char* st, size_t stlen, unsigned uin, int appid, char* gtkey_st);
            int DecoGTKey_B2(const char* b2, size_t b2len, unsigned uin, int appid, char* gtkey_b2);
        private:

            char* GetGenerateKey(int appid);
    };

}

#endif /* CQMFCRYPTOR_H_ */
