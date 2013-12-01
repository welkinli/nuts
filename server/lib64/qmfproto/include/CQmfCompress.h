/*
 * @file CQmfCompress.h
 * @brief Qmf协议的压缩类
 * @author himzheng
 * @date 2012-7-17
 */

#ifndef CQMFCOMPRESS_H_
#define CQMFCOMPRESS_H_

#include "qmf_protocal_inter.h"

namespace QMF
{

    ///服务端回包时至少需要多少字节才进行压缩
#define SVR_NEED_COMPRESS_SIZE  1024

class CQmfCompress
{
    public:
        CQmfCompress(){};
        virtual ~CQmfCompress(){};

        /**
         * 压缩
         * @param head
         * @param input
         * @param input_length
         * @param output
         * @param output_length
         * @return
         */
        int Compress(QMF_PROTOCAL::QmfHead &head,
                            const char* input, size_t input_length,
                            char* output, size_t* output_length);
        /**
         * 解压缩
         * @param head
         * @param compressed
         * @param compressed_length
         * @param output
         * @param output_length
         * @return
         */
        int UnCompress(QMF_PROTOCAL::QmfHead &head,
                            const char* compressed, size_t compressed_length,
                            char* output, size_t* output_length);

    protected:
        /*
        @brief 测试一个数某一位是否为
        @param 被测试数据
        @param 测试位数
        @return false:0 true: 1
        */
        bool BitTest(uint32_t const *Base, int Offset)
        {
             return ((*Base>>Offset)&0x01) == 1;
        }

};

/**
 * 用于服务端压缩与解压缩
 */
class CQmfCompressSvr : public CQmfCompress
{
    public:
        CQmfCompressSvr(){};
        virtual ~CQmfCompressSvr(){};

        int Compress(QMF_PROTOCAL::QmfHead &head,
                            const char* input, size_t input_length,
                            char* output, size_t* output_length);
        int UnCompress(QMF_PROTOCAL::QmfHead &head,
                            const char* compressed, size_t compressed_length,
                            char* output, size_t* output_length);
};

/**
 * 用于客户端压缩与解压缩
 */
class CQmfCompressCli : public CQmfCompress
{
    public:
        CQmfCompressCli(){};
        virtual ~CQmfCompressCli(){};

        int Compress(QMF_PROTOCAL::QmfHead &head,
                            const char* input, size_t input_length,
                            char* output, size_t* output_length);
        int UnCompress(QMF_PROTOCAL::QmfHead &head,
                            const char* compressed, size_t compressed_length,
                            char* output, size_t* output_length);
};


} // namespace QMF

#endif /* CQMFCOMPRESS_H_ */
