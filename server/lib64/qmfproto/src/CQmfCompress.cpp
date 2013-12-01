/*
 * @file CQmfCompress.cpp
 * @brief 
 * @author himzheng
 * @date 2012-7-17
 */

#include "CQmfCompress.h"
#include "zlib.h"
#include "snappy.h"

namespace QMF
{

#define BIT(bit_shift)      (1 << (bit_shift))
#define set_bit(val, bit)   ((val) |= BIT(bit))
#define clear_bit(val, bit) ((val) &= ~BIT(bit))

int CQmfCompress::Compress(QMF_PROTOCAL::QmfHead &head,
                    const char* input, size_t input_length,
                    char* output, size_t* output_length)
{
	/*
    int ret;
    if (BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_ALGORITHM_SNAPPY))
    {
        string value;
        *output_length = snappy::Compress(input, input_length, &value);
        if (*output_length < value.size())
            return -1;
        head.ComprLen = input_length;
        memcpy(output, value.data(), value.size());
        return 0;
    }
    else if (BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_ALGORITHM_ZLIB))
    {
        ret = compress((Bytef*)output, (uLongf*)output_length, (const Bytef*)input, input_length);
        if (ret != Z_OK)
            return -2;
        head.ComprLen = input_length;
        return 0;
    }*/
    return -99;
}

int CQmfCompress::UnCompress(QMF_PROTOCAL::QmfHead &head,
                    const char* input, size_t input_length,
                    char* output, size_t* output_length)
{/*
    int ret;
    if (BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_ALGORITHM_SNAPPY))
    {
        string value;
        if (!snappy::Uncompress(input, input_length, &value))
        {
            return -3;
        }
        if (*output_length < value.size()) return -2;
        memcpy(output, value.data(), value.size());
        return 0;
    }
    else if (BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_ALGORITHM_ZLIB))
    {
        ret = uncompress((Bytef*)output, (uLongf*)output_length, (const Bytef*)input, (uLong)input_length);
        if (ret != Z_OK)
           return ret;
        return 0;
    }*/
    return -99;

}

int CQmfCompressSvr::Compress(QMF_PROTOCAL::QmfHead &head,
                    const char* input, size_t input_length,
                    char* output, size_t* output_length)
{
	/*
    ///返回包不需要压缩
    if (!BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_DOWNCOMPRESS)) {
        return 1;
    }
    ///回包太小了不压缩
    if (input_length < SVR_NEED_COMPRESS_SIZE) {
        //去掉压缩标记
        head.Flag = head.Flag & ~(0x01<<(int)QMF_PROTOCAL::ENUM_QMFHEAD_F_DOWNCOMPRESS);
        return 2;
    }*/
    return CQmfCompress::Compress(head, input, input_length, output, output_length);
}

int CQmfCompressSvr::UnCompress(QMF_PROTOCAL::QmfHead &head,
                    const char* input, size_t input_length,
                    char* output, size_t* output_length)
{
	/*
    ///请求包没有压缩
    if(!BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_UPCOMPRESS)){
        return 1;
    }
	*/
    return CQmfCompress::UnCompress(head, input, input_length, output, output_length);
}

int CQmfCompressCli::Compress(QMF_PROTOCAL::QmfHead &head,
                    const char* input, size_t input_length,
                    char* output, size_t* output_length)
{
/*
    ///请求包不需要压缩
    if (!BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_UPCOMPRESS)) {
        return 1;
    }
*/
    return CQmfCompress::Compress(head, input, input_length, output, output_length);
}

int CQmfCompressCli::UnCompress(QMF_PROTOCAL::QmfHead &head,
                    const char* input, size_t input_length,
                    char* output, size_t* output_length)
{
	/*
    ///返回包没有压缩
    if(!BitTest(&head.Flag, (int)QMF_PROTOCAL::ENUM_QMFHEAD_F_DOWNCOMPRESS)){
        return 1;
    }*/
    return CQmfCompress::UnCompress(head, input, input_length, output, output_length);
}



} // namespace QMF
