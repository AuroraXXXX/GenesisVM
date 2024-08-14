//
// Created by aurora on 2024/2/1.
//

#ifndef KERNEL_METASPACE_CONSTANTS_HPP
#define KERNEL_METASPACE_CONSTANTS_HPP

#include "plat/constants.hpp"
#include "plat/utils/robust.hpp"
class CharOStream;

namespace metaspace {
    using SegementLevel_t = int8_t;
    enum class SegmentLevel: SegementLevel_t {
        LV_INVALID = -1,
        LV_ROOT = 0,
        LV_16M = LV_ROOT,
        LV_8M,
        LV_4M,
        LV_2M,
        LV_1M,
        LV_512K,
        LV_256K,
        LV_128K,
        LV_64K,
        LV_32K,
        LV_16K,
        LV_8K,
        LV_4K,
        LV_2K,
        LV_1K,
        LV_NUM,//数量
        LV_LOWEST = LV_16M,
        LV_HIGHEST = LV_1K
    };
    /**
     * Region的字节数
     * 1个Volume是Region的整倍数
     */
    constexpr inline size_t RegionBytes = 16 * M;
    /**
     * 虚拟节点保留的进程地址空间 默认的大小
     * 但是由于元空间采用伙伴合并算法
     * 所以这个大小必须是根块的2的幂次倍
     */
    constexpr inline size_t VolumeDefaultBytes = 4 * RegionBytes;
    constexpr inline size_t CommitGranuleBytes = 64 * K;
    /**
     * 从metaspace申请的内存对齐宽度
     * 注意必须 >= LogBytesPerWord
     */
    constexpr inline int32_t LogMetaAlignedBytes = LogBytesPerWord;
    constexpr inline int32_t MetaAlignedBytes = 1 << LogMetaAlignedBytes;
    static_assert(VolumeDefaultBytes % RegionBytes == 0);
    static_assert(RegionBytes % CommitGranuleBytes == 0);
    static_assert(CommitGranuleBytes % MetaAlignedBytes == 0);
    /**
     * 当从一个块分配时，
     * 如果块中的剩余区域太小而无法容纳请求的大小，
     * 我们将尝试将块大小增加一倍…
     */
    constexpr inline bool EnlargeSegmentInPlace = true;

    /**
     * @param out 打印内部使用的配置信息
     */
    extern void print_on_using_constants_setting(CharOStream *out);

    /**
     * 根据需求的字节数 计算出元空间实际应该分配的字节数
     * @param requested_bytes 需求的字节数
     * @return 实际的字节数
     */
    extern size_t get_raw_byte_for_requested(size_t requested_bytes);
    /**
     * 判断 日志级别是否是合法的
     * @param level 日志的级别
     * @return
     */
    inline bool level_is_valid(SegmentLevel level) {
        return level <= SegmentLevel::LV_HIGHEST && level >= SegmentLevel::LV_LOWEST;
    };
    #define SEGMENT_LV_FORMAT "lv%.02d"

    inline size_t level_to_bytes(SegmentLevel type) {
        assert(level_is_valid(type), "segment level is invalid.");
        return RegionBytes >> (SegementLevel_t)type;
    };
    /**
     * 将字节转换成最接近的Segment级别
     * @param bytes
     * @return
     */
    extern SegmentLevel bytes_to_level(size_t bytes);
}
#endif //KERNEL_METASPACE_CONSTANTS_HPP
