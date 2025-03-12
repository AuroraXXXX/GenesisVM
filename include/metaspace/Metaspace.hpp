//
// Created by aurora on 2025/3/11.
//

#ifndef GENESISVM_METASPACE_HPP
#define GENESISVM_METASPACE_HPP

#include "platform/mem/AllStatic.hpp"
#include "platform/constants.hpp"
class CharOStream;
namespace metaspace {
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
    /**
     * 安全性的校验
     */
    static_assert(VolumeDefaultBytes % RegionBytes == 0);
    static_assert(RegionBytes % CommitGranuleBytes == 0);
    static_assert(CommitGranuleBytes % MetaAlignedBytes == 0);
}
class Metaspace : public AllStatic {
public:

    /**
     * 用于设置元空间的参数
     */
    static void ergo_initialize();

    /**
     * 进行元空间的实际初始化
     */
    static void global_initialize();

    /**
     * 待元空间完成实际初始化后 进行一些后置的操作
     */
    static void post_initialize();

    /**
     * 尝试清除元空间中的内存
     * 应在类被卸载时候使用
     */
    static void purge();


    /**
     * 打印元空间的基本信息
     * @param out
     */
    static void print_on(CharOStream *out);

    /**
     * 根据需求的字节数 计算出元空间实际应该分配的字节数
     * @param requested_bytes 需求的字节数
     * @return 实际的字节数
     */
    static size_t get_meta_bytes_aligned(size_t requested_bytes);
};

#endif //GENESISVM_METASPACE_HPP
