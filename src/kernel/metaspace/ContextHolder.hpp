//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_CONTEXT_HOLDER_HPP
#define KERNEL_METASPACE_CONTEXT_HOLDER_HPP

#include "plat/mem/allocation.hpp"
#include "kernel/metaspace/constants.hpp"
#include "SegmentManager.hpp"
#include "VolumeList.hpp"
#include "plat/utils/OrderAccess.hpp"


namespace metaspace {
    class Segment;


    class ContextHolder : public CHeapObject<MEMFLAG::Metaspace> {
    private:
        VolumeList *const _volume_list;
        /**
         * 用于实际上管理的内存块
         */
        SegmentManager *const _segment_mgr;
        /**
         * 静态的全局对象
         */
        static ContextHolder *_context;
        /**
         * 实际使用的字节，所有的Arena
         */
        volatile size_t _used_bytes;

        /**
         * 在空闲的内存块中搜寻满足要求的
         * @param preferred_level 希望的内存块等级
         * @param max_level 最大的内存块等级
         * @param suggest_min_committed_bytes 建议的最小提交内存
         * @return
         */
        Segment *search_satisfy_segment_in_free(
                SegmentLevel preferred_level,
                SegmentLevel max_level,
                size_t suggest_min_committed_bytes);

        /**
         * 切割内存块 并管理切割下来的内存块
         * 增加InternalStats内部的统计信息
         * @param chunk
         * @param target_level 目标内存块等级
         */
        void split_segment(Segment *segment, SegmentLevel target_level);

        /**
         * 尝试合并空闲的块
         * @param chunk
         * @return 如果发生合并 返回的是合并后的块地址
         *         如果没有发生合并 那么返回的是传入的块地址
         */
        Segment *attempt_merge_segment(Segment *segment);

        /**
         * 归还块的主体逻辑
         * @param chunk
         */
        void return_segment_with_lock(Segment *segment);

        /**
         * 构造一个全局的空闲块 管理器
         */
        explicit ContextHolder(
                SegmentManager *segment_mgr,
                VolumeList *volume_list);

        ~ContextHolder();

    public:

        inline void add_arena_used_bytes(size_t bytes) {
            OrderAccess::fetch_and_add(&this->_used_bytes, bytes);
        };

        inline void sub_arena_used_bytes(size_t bytes) {
            OrderAccess::fetch_and_sub(&this->_used_bytes, bytes);
        };

        static inline ContextHolder *context() {
            assert(_context != nullptr, "未初始化");
            return _context;
        };


        /**
         * 初始化全局代码
         */
        static void init_context();

        /**
         * 将内存块 添加到 SegmentManager
         * 并合并相邻的内存块
         * 首先会重置内部数据
         * 之后用户不能再访问这些内存块
         * @param segment
         */
        void return_segment(Segment *segment);

        /**
         * 内部需要获取元空间锁
         *
         * 如果成功,至少返回一个max_level级别的内存块,
         *  有宽裕条件会返回preferred_level的内存块
         *  且min_committed_bytes字节 保证被提交
         * 内部首先会获取元空间的锁
         *
         * 如果失败,原因:
         * 1)本身是压缩类空间,且保留的虚拟进程地址空间已被使用完毕,无法被扩展
         * 2)达到内存提交的阈值无法提交内存,阈值有GC阈值和MaxMetaspaceSize
         * @param preferred_level 希望的内存块等级
         * @param max_level (最少内存块大小)最大内存块等级
         * @param min_committed_bytes 最少应该被提交的内存大小 单位字节
         * @return
         */
        Segment *get_segment(SegmentLevel preferred_level,
                             SegmentLevel max_level,
                             size_t min_committed_bytes);

        /**
         * 尝试扩展块 但是这个块必须不是根块
         * 第一步就是获取元空间的锁
         * @param chunk
         * @return 合并是否成功
         */
        bool attempt_enlarge_segment(Segment *segment);

        /**
         * 用于清理
         */
        void purge();

        /**
         * 统计为元空间保留下来的进程空间
         * 统计的信息来自于 VolumeList
         * @return
         */
        inline size_t reserved_bytes() {
            return this->_volume_list->reserved_bytes();
        };

        /**
        * -------------------------
        * 统计 提交的内存大小
        * 统计的信息来自于 VolumeList
        * @return 单位 字节
        */
        inline size_t committed_bytes() {
            return this->_volume_list->committed_bytes();
        };

        /**
         * -------------------------
         * 统计 空闲的内存块大小
         * 采集的信息来自于 SegmentManager的统计
         * @return 单位 字节
         */
        inline size_t free_bytes() {
            return this->_segment_mgr->total_bytes();
        };

        /**
         * 统计 所有Arena的实际使用的字节
         * @return
         */
        inline size_t used_bytes() {
            return OrderAccess::load(&this->_used_bytes);
        };

        /**
         * 打印函数
         * @param out
         */
        void print_on(CharOStream *out) const;

#ifdef DIAGNOSE
      static void verify();
#endif
    };
}

#endif //KERNEL_METASPACE_CONTEXT_HOLDER_HPP
