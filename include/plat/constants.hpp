//
// Created by aurora on 2024/6/25.
//

#ifndef PLATFORM_CONSTANTS_HPP
#define PLATFORM_CONSTANTS_HPP
#include "stdtype.hpp"
enum class OSReturn {
    OK = 0,
    ERR = -1,//其他的错误
    INTRPT = -2, //被中断了
    TIMEOUT = -3,//超时了
    NO_MEMORY = -4,//没有内存 内存不足
    NO_RESOURCE = -5//缺少不是内存相关的资源
};

/**
 * VM 内部使用到的强制性约束
 * 不应该被修改
 * 修改会导致VM未知的错误
 */
constexpr inline uint8_t LogKB = 10;
constexpr inline uint8_t LogMB = 20;
constexpr inline uint8_t LogGB = 30;
constexpr inline uint8_t LogTB = 40;
constexpr inline size_t K = 1L << LogKB;
constexpr inline size_t M = 1L << LogMB;
constexpr inline size_t G = 1L << LogGB;
constexpr inline size_t T = 1L << LogTB;
/**
 * 时间单位的换算
 */
constexpr inline uint32_t TicksPerNS = 1;
constexpr inline uint32_t TicksPerUS = 1e3;
constexpr inline uint32_t TicksPerMS = 1e6;
constexpr inline uint32_t TicksPerS = 1e9;
constexpr inline int32_t SPerMin = 60;
constexpr inline int32_t MinPerHour = 60;
/**
 * ---------------------
 * 每个字具有的字节数 进行log2计算后得到的数值
 * ---------------------
 */
constexpr inline int32_t LogBytesPerWord = 3;
/**
 * 每个字节具有的比特数 进行log2计算后得到的数值
 */
constexpr inline int32_t LogBitsPerByte = 3;
constexpr inline int32_t LogBitsPerWord = LogBytesPerWord + LogBitsPerByte;
constexpr inline int32_t BytesPerWord = 1 << LogBytesPerWord;
constexpr inline int32_t BitsPerByte = 1 << LogBitsPerByte;
constexpr inline int32_t BitsPerWord = 1 << LogBitsPerWord;


static_assert(sizeof(size_t) == sizeof(long) &&
              sizeof(long) == BytesPerWord &&
              sizeof(void *) == BytesPerWord,
              "only support 64 bit plat");

/**
 * 最大的优先级和最小优先级
 * 都是包含的
 * 此处的定义需要与Flags中的标志定义相同
 */
enum ThreadPriority: int16_t {
    NoPriority = -1,// Initial non-priority value
    MinPriority = 1,
    NormPriority = 5,
    NearMaxPriority = 9,
    MaxPriority = 10,
    TotalPriority = 11
};

/**
 * ---------------------
 * 自旋锁默认设置
 * ---------------------
 */
/**
 * 默认自旋的次数
 */
constexpr inline uint32_t SpinDefaultSpinLimit = 4096;
/**
 * 超过自旋的次数 经执行让出CPU的操作
 */
constexpr inline uint32_t SpinDefaultYieldLimit = 64;
/**
 * 超出让出CPU次数后 进行睡眠
 */
constexpr inline uint32_t SpinDefaultSleepNs = 1000;
/**
 * OStream输出缓冲区的字节数
 * 默认的
 */
constexpr inline uint32_t OStreamDefaultBufSize = 2000;
/**
 * iso8601 格式化所需的 最小缓冲区的大小
 * "YYYY-MM-DDThh:mm:ss.mmm+zz:zz"
 * "YYYY-MM-DDThh:mm:ss.mmmZ" UTC时间
 */
constexpr inline int16_t ISO8601_BUF_SIZE = 30;
/**
 * 32有符号数 10进制 所需的缓冲区
 * 包括所需的正负号
 */
constexpr inline int16_t INT32_DEC_BUF_SIZE = 12;
/**
 * 64有符号数 10进制 所需的缓冲区
 * 包括所需的正负号
 */
constexpr inline int16_t UINT64_DEC_BUF_SIZE = 22;

#endif //PLATFORM_CONSTANTS_HPP
