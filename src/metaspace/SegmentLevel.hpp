#ifndef METASPACE_SEGMENT_LEVEL_HPP
#define METASPACE_SEGMENT_LEVEL_HPP

#include "platform/allocation.hpp"

#define SEGMENT_LV_FORMAT "lv%.02d"
namespace metaspace {
    /**
     * 定义Segment的级别信息
     */
    using SegmentLevel_t = int8_t;

    class SegmentLevel : public AllStatic {
    public:
        /**
         * 定义Segment的级别所存储的类型
         */

        enum : SegmentLevel_t {
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

    public:
        /**
         * 根据字节数 获取 最合适的 SegmentLevel
         * @param bytes 所需的字节数
         * @return level对应的内存块大小 >= bytes ,且是最小的。
         */
        static SegmentLevel_t get_optimal_level(size_t bytes);

        /**
         * 获取SegmentLevel对应的字节数
         */
        static size_t get_bytes(SegmentLevel_t level);

        /**
         * 判断当前level是否合法
         * @param level
         * @return
         */
        static bool is_valid(SegmentLevel_t level) {
            return level >= LV_LOWEST && level <= LV_HIGHEST;
        };
    };

}

#endif //METASPACE_SEGMENT_LEVEL_HPP