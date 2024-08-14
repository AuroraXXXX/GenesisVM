//
// Created by aurora on 2022/12/23.
//

#ifndef KERNEL_METASPACE_SEGMENT_HEADER_POOL_HPP
#define KERNEL_METASPACE_SEGMENT_HEADER_POOL_HPP

#include "plat/mem/allocation.hpp"
#include "Segment.hpp"
#include "kernel/utils/LinkedList.hpp"

namespace metaspace {
    /**
     * 用于管理所有的Segment的内存块头部信息，即这个对象本身
     */
    class SegmentHeaderPool : public CHeapObject<MEMFLAG::Metaspace> {
    private:
        constexpr inline static int SlabCapacity = 128;

        struct Slab : public CHeapObject<MEMFLAG::Metaspace> {
            Slab *_next;
            Segment _elems[SlabCapacity];
            explicit Slab() :
                    _next(nullptr),
                    _elems() {
            };
        };

        /**
         * _slab_nums 表示当前总共申请得到Slab的数量
         * _dead_segments_num 表示其管理的MetaChunk数量，即 _dead_chunk 形成链表的长度
         * _slab_top 表示下一次申请内存块头部信息时候，位于Slab::_elems的索引
         *              即下一次的内存块头部 是 Slab->elems[_slab_top]
         *  _used_headers 被使用的内存块头部的数量
         *  _first_slab 表示Slab形成的链表用于释放内存时候使用
         *              这个链表中第一块是正在使用的 之后的都是被使用完毕的
         */
        int _slab_nums;
        int _dead_segments_num;
        int _slab_top;
        int _used_headers;
        LinkList<Segment> _dead_segments;
        Slab *_first_slab;

        /**
         * 申请得到一块新的Slab内存
         */
        void allocate_new_slab();

        static SegmentHeaderPool *_pool;

        explicit SegmentHeaderPool();
    public:
        /**
         * 析构函数 释放申请到的内存
         */
        ~SegmentHeaderPool();

        /**
         * 申请得到一块内存头部信息
         * 并且会把原来的数据擦除
         * @return
         */
        Segment *allocate_segment_header();

        /**
         * 归还一个内存头部
         * 原本的部分数据并不会擦除
         * 所以用户不可以访问这些数据
         * @param chunk
         */
        void deallocate_segment_header(Segment *chunk);

        /**
         * 获取内存块头部的池
         * @return
         */
        static inline SegmentHeaderPool *pool() {
            return SegmentHeaderPool::_pool;
        };

        /**
         * 初始化
         */
        static void initialize();
    };
}


#endif //KERNEL_METASPACE_SEGMENT_HEADER_POOL_HPP
