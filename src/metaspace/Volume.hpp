//
// Created by aurora on 2022/12/16.
//

#ifndef METASPACE_VOLUME_HPP
#define METASPACE_VOLUME_HPP

#include "platform/allocation.hpp"
#include "platform/utils/Space.hpp"
#include "CommittedBitMap.hpp"
#include <atomic>
namespace metaspace {
    class Segment;

    class RootArea;
    /**
     * 最粗力度的元空间内存管理单位
     * 仅仅保留进程地址空间 并不进行内存的分配
     * 基于伙伴分配算法 应该是Root Segment的整数倍
     */
    class Volume : public CHeapObject<MEMFLAG::Metaspace> {
        friend class RootArea;

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
        CommittedBitMap _commit_bitmap;

        /**
         * 用于统计相应的内存情况
         */
        std::atomic<size_t> *const _committed_statistics;


    public:
        /**
         * 构造函数
         * 保留的地址空间大小应该按照物理页对齐
         * @param virtual_space 保留的虚拟地址空间
         * @param reserved_statistics
         * @param committed_statistics 用于统计的内存提交情况
         */
        explicit Volume(Space &virtual_space,
                        std::atomic<size_t> *committed_statistics);

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
        [[nodiscard]] size_t committed_bytes_slow_path() const {
            return this->_commit_bitmap.get_committed_bytes();
        };

        /**
         * 获取Volume覆盖的地址空间的大小
         * @return
         */
        [[nodiscard]] inline size_t reserved_bytes() const {
            return this->_reserved.capacity_bytes();
        };

        /**
         * 是否包含指定的虚拟地址
         * @param p
         * @return
         */
        inline bool contain(void* p) {
            return this->_reserved.contains(p);
        };

#ifdef DEBUG_MODE_ONLY
        void verify() const;
#endif

        /**
         * 输出本节点的信息
         * @param out
         */
        void print_on(CharOStream *out);
    };
}

#endif //METASPACE_VOLUME_HPP
