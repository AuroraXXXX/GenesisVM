//
// Created by aurora on 2022/12/25.
//

#ifndef KERNEL_METASPACE_COMMITTED_MASK_HPP
#define KERNEL_METASPACE_COMMITTED_MASK_HPP

#include "kernel/utils/BitMap.hpp"
#include "kernel/metaspace/constants.hpp"
#include "kernel/utils/Space.hpp"

namespace metaspace {
    /**
     * 使用统计区间(bitmap 一小段内存)
     * 来统计 被映射区间 的 实际提交状况
     *
     * 即用于统计虚拟节点内存实际提交的状况
     *
     * 统计区间每一位可以表示被映射区间的一段内存
     * 是否提交，这段被映射区间大小被称为统计粒度
     *
     * 统计区间中每一位:
     * 1 表示提交
     * 0 表示未提交
     */
    class CommittedMask : public CHeapBitMap {
    private:
        /**
         * 表示被映射的区间[_base,_base + mapping_range_size() )
         */
        uintptr_t _base;

        /**
        * 比特数组中每一个比特位可以表示多大的被映射区间是否提交的状况
        * 单位是字节
        * 同时称之为 统计粒度
        */
        inline static size_t statistics_bytes_per_bit() {
            return metaspace::CommitGranuleBytes;
        };

        /**
         * 根据地址p获取统计区间的比特位序号
         * @param p 地址
         * @return
         */
        size_t bit_no_for_address(void *p) const {

            auto bit_no = ((uintptr_t) p - this->_base) /
                          CommittedMask::statistics_bytes_per_bit();
            assert(this->is_within(bit_no), "is out of committed mask");
            return bit_no;
        };

    public:
        /**
         * 被映射区间的大小 单位字节
         * @return
         */
        [[nodiscard]] inline size_t mapping_range_bytes() const {
            return this->total_bits() * CommittedMask::statistics_bytes_per_bit();
        };

        /**
        * 构造函数
        * @param range_start
        * @param range_length
        */
        explicit CommittedMask(void *range_start, size_t range_length);

        inline explicit CommittedMask(Space &space) :
                CommittedMask(space.start(), space.capacity_bytes()) {
        };

        /**
         * 获取被映射区间啊[range_start,range_start + range_bytes) 的内存提交情况
         * @param range_start
         * @param range_bytes
         * @return
         */
        size_t get_committed_bytes_in_range(void *range_start,
                                            size_t range_bytes) const;

        /**
         * 获取整个统计区间所有提交内存的大小
         * @return
         */
        [[nodiscard]] inline size_t get_committed_bytes() const {
            return this->count_range(0, this->total_bits())
                   * CommittedMask::statistics_bytes_per_bit();
        };

        /**
         * 将被映射区间[range_start,range_start+range_bytes) 的内存提交状况设置为未提交
         * 并返回此次操作之前 未提交内存的大小 单位字节
         * @param range_start 区间的起始地址
         * @param range_bytes 区间的长度
         */
        void mark_range_as_uncommitted(void *range_start,
                                       size_t range_bytes);

        /**
         * 将被映射区间[range_start,range_start+range_bytes)的内存提交状况设置为 提交
         * 并返回此次操作之前 提交内存的大小 单位字节
         * @param range_start 区间的起始地址
         * @param range_bytes 区间的长度
         */
        void mark_range_as_committed(void *range_start,
                                     size_t range_bytes);

        /**
         * 打印内部统计区间中情况
         * @param out 输出流
         * @param committed_char 提交状态的输出字符
         * @param uncommitted_char 未提交状态的输出字符
         */
        void print_on(CharOStream *out,
                      char committed_char = 'X',
                      char uncommitted_char = '-') const;
    };
}
#endif //KERNEL_METASPACE_COMMITTED_MASK_HPP
