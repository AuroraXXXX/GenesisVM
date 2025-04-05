//
// Created by aurora on 2022/12/23.
//

#ifndef METASPACE_SEGMENT_HEADER_POOL_HPP
#define METASPACE_SEGMENT_HEADER_POOL_HPP

#include "platform/allocation.hpp"
#include "Segment.hpp"
#include <atomic>
#include "platform/utils/LinkList.hpp"
#include "platform/utils/LinkStack.hpp"
namespace metaspace {
    /**
     * 用于管理所有的Segment的内存块头部信息，即这个对象本身
     */
    class SegmentHeaderPool : public CHeapObject<MEMFLAG::Metaspace> {
    private:
        constexpr inline static int SlabCapacity = 128;
        /**
         * 1个Slab中可以分配出多个SegmentHeader
         */
        struct Slab : public CHeapObject<MEMFLAG::Metaspace> {
        private:
            Slab *_next;
        public:
            /**
             * 用于分配SegmentHeader
             */
            Segment _elems[SlabCapacity];
            explicit Slab() :
                    _next(nullptr),
                    _elems() {
            };
            inline auto next(){
                return this->_next;
            };
            inline void set_next(Slab *next){
                this->_next = next;
            }
        };

        /**
         * 当前正在使用的Slab中，下一个可以分配的位置
         */
        std::atomic<uint32_t> _slab_top;
        /**
         * 已经被分配出去的
         */
        std::atomic<uint32_t> _used_headers;
        /**
         * 管理死亡的头部
         */
        LinkList<Segment> _dead_headers;
        /**
         * 管理已经申请到的Slab
         * 栈顶表示正式使用的
         * 之后的表示已经使用完毕的
         */
        LinkStack<Slab> _slab_list;

        /**
         * 申请得到一块新的Slab内存
         */
        void allocate_new_slab();

        static SegmentHeaderPool *_pool;

        explicit SegmentHeaderPool();
        /**
         * 析构函数 释放申请到的内存
         */
        ~SegmentHeaderPool();
    public:

        /**
         * 申请得到一块内存头部信息
         * 并且会把原来的数据擦除
         * @return
         */
        Segment *alloc();

        /**
         * 归还一个内存头部
         * 原本的部分数据并不会擦除
         * 所以用户不可以访问这些数据
         * @param segment
         */
        void free(Segment *segment);
        /**
         * 获取当前已经分配出去的内存块头部 数量
         * @return
         */
        [[nodiscard]] inline auto used_headers() const {
            return this->_used_headers.load();
        };
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


#endif //METASPACE_SEGMENT_HEADER_POOL_HPP
