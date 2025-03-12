
#include "platform/constants.hpp"

namespace metaspace {
    class Setting
    {
    public:
        /**
         * Region的字节数
         * 1个Volume是Region的整倍数
         */
        constexpr inline static size_t RegionBytes = 16 * M;
        /**
         * 虚拟节点保留的进程地址空间 默认的大小
         * 但是由于元空间采用伙伴合并算法
         * 所以这个大小必须是根块的2的幂次倍
         */
        constexpr inline static size_t VolumeDefaultBytes = 4 * RegionBytes;
        constexpr inline static size_t CommitGranuleBytes = 64 * K;
        /**
         * 从metaspace申请的内存对齐宽度
         * 注意必须 >= LogBytesPerWord
         */
        constexpr inline static int32_t LogMetaAlignedBytes = LogBytesPerWord;
        constexpr inline static int32_t MetaAlignedBytes = 1 << LogMetaAlignedBytes;
        constexpr inline static int32_t MinMetaBytes = LogBytesPerWord;
        /**
         * 安全性的校验
         */
        static_assert(VolumeDefaultBytes % RegionBytes == 0);
        static_assert(RegionBytes % CommitGranuleBytes == 0);
        static_assert(CommitGranuleBytes % MetaAlignedBytes == 0);
        static_assert(MetaAlignedBytes >= LogBytesPerWord);
    };


}