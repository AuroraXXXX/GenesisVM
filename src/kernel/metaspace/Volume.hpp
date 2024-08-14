//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_VOLUME_HPP
#define KERNEL_METASPACE_VOLUME_HPP

#include "plat/mem/allocation.hpp"
#include "kernel/utils/Space.hpp"
#include "CommittedMask.hpp"
#include "Region.hpp"

namespace metaspace {
    class Segment;

    class ContextHolder;

    /**
     * 最粗力度的元空间内存管理单位
     * 仅仅保留进程地址空间 并不进行内存的分配
     * 基于伙伴分配算法 应该是Root Segment的整数倍
     */
    class Volume : public CHeapObject<MEMFLAG::Metaspace> {
    private:
        /**
         * 指向下一个 虚拟节点，用于维持链表
         */
        Volume *_next;
        /**
         * 保留下来的进程空间
         */
        Space _reserved;
        /**
         * 统计 整个映射区间的内存提交状态
         */
        CommittedMask _commit_mask;
        /**
         * 根块的总共数量
         */
        const uint16_t _total_region_num;
        /**
         * 下一次可分配的Region索引
         * 整个数值只会增加 即使之前的Region变成空闲的
         * 我们也无法进行统计到，视为使用完了
         */
        uint16_t _next_region_index;
        /**
         * 用于统计相应的内存情况
         */
        size_t *const _committed_statistics;
        Region *_region;

        /**
         * 获取某一个 region 的地址
         * 编号介于[0,_total_region_num)之间
         * @param index region 索引
         * @return
         */
        inline Region *region_by_index(uint16_t index) {
            assert(index < this->_total_region_num, "out of region index");
            return this->_region + index;
        };


        /**
         * 判断整个 region 是否全部是空闲的
         * @return
         */
        bool total_region_is_free();


    public:
        /**
         * 构造函数
         * 保留的地址空间大小应该按照物理页对齐
         * @param virtual_space 保留的虚拟地址空间
         * @param reserved_statistics
         * @param committed_statistics 用于统计的内存提交情况
         */
        explicit Volume(Space &virtual_space,
                        size_t *committed_statistics);

        /**
         * 析构函数
         * 1 解除当前Volume 覆盖的进行地址空间的映射
         * 即将这部分的映射内存完全释放
         * 2 将本Volume对象自身 使用到的内存完全释放掉
         */
        ~Volume();

        /**
         * 设置下一个Volume
         * @param node
         */
        inline void set_next(Volume *node) {
            this->_next = node;
        };

        [[nodiscard]] inline Volume *next() const {
            return this->_next;
        };

        /**
         * 获取整个Volume内存提交的情况 单位字节
         * @return
         */
        [[nodiscard]] size_t committed_bytes() const {
            return this->_commit_mask.get_committed_bytes();
        };

        /**
         * 获取Volume覆盖的地址空间的大小
         * @return
         */
        [[nodiscard]] inline size_t reserved_bytes() const {
            return this->_reserved.capacity_bytes();
        };

        /**
         * 表示分配出去的虚拟空间大小
         * @return
         */
        [[nodiscard]] size_t used_bytes() const {
            return this->_next_region_index * RegionBytes;
        };


        /**
         * 分配一个root segment
         * 必须在获取元空间锁的情况下 才可以调用这个函数
         * @return
         */
        Segment *allocate_root_segment();

        /**
         * 是否还存在空闲的region
         * @return
         */
        [[nodiscard]] inline bool has_unused_region() const {
            return this->_next_region_index < this->_total_region_num;
        };

        /**
         * 通过指针 获取覆盖这个region
         * @param p 指针
         * @return
         */
        Region *region_by_pointer(void* p);

        /**
         * 如果当前当前区间已经完全提交 不存在未提交的部分 我们是不会进行提交的
         *
         * 但是如果当前区间存在可提交的部分 我们会对整个区间进行提交
         * 即使用一个新的映射来替换现有的映射
         * 因此之前已提交部分的现有内存将会被擦除
         *
         * @param p 提交内存的首地址 与提交粒度(CommitGranuleBytes)对齐
         * @param bytes 提交的大小 与提交粒度(CommitGranuleBytes)对齐
         * @return 提交是否成功
         */
        bool commit_range(void* p, size_t bytes);

        /**
         * 将[p,p+bytes)区间的内存释放掉
         * @param p
         * @param bytes
         */
        void uncommit_range(void* p, size_t bytes);

        /**
         * 是否包含指定的虚拟地址
         * @param p
         * @return
         */
        inline bool contain(void* p) {
            return this->_reserved.contains(p);
        };

#ifdef DIAGNOSE
        void verify() const;
#endif

        /**
         * 输出本节点的信息
         * @param out
         */
        void print_on(CharOStream *out);
    };
}

#endif //KERNEL_METASPACE_VOLUME_HPP
