//
// Created by aurora on 2022/12/23.
//

#ifndef KERNEL_METASPACE_REGION_HPP
#define KERNEL_METASPACE_REGION_HPP

#include "stdtype.hpp"
#include "kernel/metaspace/constants.hpp"

namespace metaspace {
    class Segment;

    class SegmentManager;

    class Volume;

    /**
     * 根块区域
     * 负责MetaChunk的切分和合并 以及MetaChunk的构造
     *
     * !!! 注意本类方法不负责向InternalStats汇报统计信息 !!!
     */
    class Region {
    private:
        /**
         * 保留的地址空间的首地址
         */
        uintptr_t _base;
        /**
         * 通过prev_in_vs和next_in_vs指针形成链表
         */
        Segment *_first;
    public:
        /**
         * 构造函数
         * @param base 根块保留的内存首地址
         */
        explicit Region(void *base) : _base((uintptr_t)base), _first(nullptr) {};

        /**
         * 析构函数 应该在虚拟节点被销毁时候调用
         * 归还可能调用alloc_root_chunk获取的内存块头信息
         */
        ~Region();

        /**
         * 申请获取一个根块 这个函数有VirtualSpaceNode调用
         * @param container 根块所属的虚拟节点
         * @return
         */
        Segment *alloc_root_segment(Volume *container);

        /**
         * 尝试将 正在使用中的内存块虚拟地址空间大小 x2
         * 即让内存块等级减一
         *
         * 且保证当前块的已使用的信息不变
         * @param chunk 需要被扩大的块
         * @param manager 空闲的块管理器
         * @return 块扩大是否成功
         */
        bool attempt_enlarge_segment(Segment *segment, SegmentManager *manager) const;

        /**
         * 将空闲的MetaChunk与ChunkManager中的空闲块进行合并
         * @param chunk
         * @param manager 空闲的内存块MetaChunk的管理器
         * @return 如果不为空 返回的是合并后的新内存的首地址
         *              且合并的块从ChunkManager删除
         *              !!!所有旧的内存块不应该再次被访问!!!
         *         如果为空 那么说明合并失败
         */
        Segment *merge(Segment *chunk,
                       SegmentManager *manager) const;

        /**
         * 递归的切割给定的内存块source_chunk，直到获取指定target_level的内存块
         * 切割出来多余的内存块 则交予ChunkManager进行管理
         *
         * 切割的算法是将内存块 一分为二
         * 如果不满足需求 那么将第一个内存块 再次按照切割算法一直切割
         * 直到满足需求位置
         * @param target_level 指定等级的内存块
         * @param source_chunk 原内存块
         * @param manager 空闲块管理器
         */
        void split(SegmentLevel target_level,
                   Segment *source,
                   SegmentManager *manager) const;
        /**
         * 根块区域覆盖的首地址
         * @return
         */
        [[nodiscard]] inline void *base() const {
            return (void *)(this->_base);
        };
        /**
         * 根块的结束地址
         * @return
         */
        [[nodiscard]] inline void * end() const {
            return (void *)((uintptr_t)(this->_base) + RegionBytes);
        };

        /**
         * 判断根块区域是否是空闲的
         * @return
         */
        bool is_free();

        /**
         * 获取第一个内存块
         * @return
         */
        inline Segment *first_segment() {
            return this->_first;
        };
        /**
         * 打印内部 块的信息
         * @param out
         */
        void print_on(CharOStream* out) const;

#ifdef DIAGNOSE
        void verify()const;
#endif
        /**
         * 用于debug校验
         * @param p
         */
        inline bool contain(void* p) const {
            return is_clamp<void*>(p,this->base(),this->end());
        };

    };
}


#endif //KERNEL_METASPACE_REGION_HPP
