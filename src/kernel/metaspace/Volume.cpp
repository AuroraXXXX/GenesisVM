//
// Created by aurora on 2022/12/16.
//
#include "global/flag.hpp"
#include "Volume.hpp"
#include "kernel/metaspace/constants.hpp"
#include "Region.hpp"
#include "Segment.hpp"
#include "CommittedMask.hpp"
#include "kernel_mutex.hpp"
#include "kernel/metaspace/InternalStats.hpp"
#include "meta_log.hpp"
#include "kernel/metaspace/CommittedLimiter.hpp"
#include "plat/os/mem.hpp"
#define LOG_FMT "Volume @" PTR_FORMAT " base=" PTR_FORMAT" "
#define LOG_FMT_ARGS this,this->_reserved.start()

namespace metaspace {
    Volume::Volume(Space &virtual_space,
                   size_t *committed_statistics) :
            _next(nullptr),
            _reserved(virtual_space),
            _next_region_index(0),
            _total_region_num(virtual_space.capacity_bytes() / RegionBytes),
            _committed_statistics(committed_statistics),
            _commit_mask(virtual_space) {
        assert_is_aligned<size_t>(virtual_space.capacity_bytes(), RegionBytes);
        /**
         * 设置根区域的信息
         */
        this->_region = NEW_CHEAP_ARRAY(Region, this->_total_region_num, MEMFLAG::Metaspace);
        auto root_covered_literal = this->_reserved.start_literal();
        for (uint16_t i = 0; i < this->_total_region_num; ++i) {
            new(this->_region + i) Region((void *)root_covered_literal);
            root_covered_literal +=  RegionBytes;
        }
        //新增统计信息
        InternalStats::inc_num_volumes_births();
        meta_log2(debug, "出生(born),size " SIZE_FORMAT " K", this->reserved_bytes() / K);
    }

    Volume::~Volume() {
        auto committed_bytes = this->committed_bytes();
        auto reserved_bytes = this->reserved_bytes();
        meta_log2(debug, "死亡(dies),size " SIZE_FORMAT " K", reserved_bytes / K);


        /**
         * 销毁统计内存
         */
        FREE_CHEAP_ARRAY(this->_region,MEMFLAG::Management);
       /**
         * 修改虚拟链表 内存提交的统计信息
         */
        *this->_committed_statistics -= committed_bytes;
        /**
         * 修改内部运行状态的统计信息 用户检测
         */
        InternalStats::inc_num_volumes_deaths();
    }


    Region *Volume::region_by_pointer(void* p) {
        //首先这个地址必须被虚拟节点的覆盖
        assert(this->contain(p), "非法的指针");
        //判断落入的根块区域编号
        auto idx = (uint16_t) (((uintptr_t)p - this->_reserved.start_literal()) / RegionBytes);
        return this->region_by_index(idx);
    }

    Segment *Volume::allocate_root_segment() {
        assert_lock_strong(Metaspace_lock);
        //说明被用完了 没有可用的了
        if (!this->has_unused_region()) {
            return nullptr;
        }
        //获取对应的根块区域的对象
        const auto region = this->region_by_index(this->_next_region_index++);
        //构造对应的根块
        auto segment = region->alloc_root_segment(this);
        assert(segment->is_root_segment() &&
               segment->is_free() &&
               segment->base() == region->base() &&
               segment->container() == this,
               "健全");
        return segment;
    }


    bool Volume::commit_range(void *p, size_t bytes) {
        /**
         * 首先校验要提交区间的首地址和区间大小
         * 必须都要和内存的提交粒度对齐
         */
        assert_is_aligned((size_t) p, CommitGranuleBytes);
        assert(bytes > 0 && is_aligned(bytes, CommitGranuleBytes),
               "提交区间大小非法");
        assert_lock_strong(Metaspace_lock);
        //首先计算这个范围内提交的内存有多大
        const auto committed_bytes_in_range = this->_commit_mask.
                get_committed_bytes_in_range(p, bytes);
        //计算如果成功提交内存 我们会增加多少提交内存
        const auto committed_increase_bytes = bytes - committed_bytes_in_range;

        if (committed_increase_bytes == 0) {
            //说明之前已经完全提交了 我们无需再次提交
            meta_log2(debug, "已完全提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                    SIZE_FORMAT "K.",
                      p, (void *)((uintptr_t)p + bytes), bytes / K);
            return true;
        }
        /**
         * 需要检查是否大小内存提交限制
         */
        if (CommittedLimiter::possible_expand_bytes() < committed_increase_bytes) {
            meta_log2(debug, "!!达到限制!!无法提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                    SIZE_FORMAT "K.应增加 " SIZE_FORMAT "K.",
                      p, (void *)((uintptr_t)p + bytes), bytes / K,
                      committed_increase_bytes / K);
            return false;
        }
        /**
         * 进行内存的实际提交
         */
        if (!os::commit_memory(MEMFLAG::Metaspace,p,bytes,os::CommitType::rwx)) {
            vm_exit_out_of_memory(VMErrorType::OOM_MMAP_ERROR,
                                  bytes,
                                  "为元空间(metaspace)提交内存失败");
        }
        if (global::AlwaysPreTouch) {
            os::pretouch_memory(p,  bytes);
        }
        meta_log2(debug, "提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                SIZE_FORMAT "K.实际增加" SIZE_FORMAT "K.",
                  p, (void *)((uintptr_t)p + bytes),
                  bytes / K, committed_increase_bytes / K);
        CommittedLimiter::increase_committed_bytes(committed_increase_bytes);
        /**
         * 修改虚拟节点链表 内存提交的统计信息
         */
        *this->_committed_statistics += committed_increase_bytes;
        //修改统计区间的信息
        this->_commit_mask.mark_range_as_committed(p, bytes);
        /**
         * 最后成功的话 增加统计信息
         */
        InternalStats::inc_num_range_committed();
        return true;
    }

    void Volume::uncommit_range(void* p, size_t bytes) {
        /**
         * 首先校验要提交区间的首地址和区间大小
         * 必须都要和内存的提交粒度对齐
         */
        assert_is_aligned((size_t) p, CommitGranuleBytes);
        assert_is_aligned(bytes, CommitGranuleBytes);
        assert_lock_strong(Metaspace_lock);

        //首先计算这个范围内提交的内存有多大
        const auto committed_bytes_in_range = this->_commit_mask.
                get_committed_bytes_in_range(p, bytes);
        assert_is_aligned(committed_bytes_in_range,
                                  CommitGranuleBytes);
        if (committed_bytes_in_range == 0) {
            /**
             * 说明之前已经完全撤销提交了 我们无需进行任何操作
             */
            meta_log2(debug, "已完全撤销提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                    SIZE_FORMAT "K.",
                      p, (void *)((uintptr_t)p + bytes), bytes / K);
            return;
        }
        /**
         * 下面开始实际上的取消提交
         */
        if (!os::uncommit_memory(MEMFLAG::Metaspace,p, bytes)) {
            /**
             * 如果 提交失败 那么直接中止
             * 这个的确是可能发生的 因为撤销内存提交会导致映射增加
             */
            vm_exit_out_of_memory(VMErrorType::OOM_MMAP_ERROR,
                                  bytes,
                                  "为元空间(metaspace)撤销提交内存失败");
        }
        meta_log2(debug, "撤销提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                SIZE_FORMAT "K.实际撤销" SIZE_FORMAT "K",
                  p,
                  (void *)((uintptr_t)p + bytes),
                  bytes / K,
                  committed_bytes_in_range / K);
        CommittedLimiter::decrease_committed_bytes(committed_bytes_in_range);
        /**
         * 修改虚拟节点链表 内存提交的统计信息
         */
        *this->_committed_statistics -= committed_bytes_in_range;
        //更新统计区间
        this->_commit_mask.mark_range_as_uncommitted(p, bytes);
        //更新性能信息统计
        InternalStats::inc_num_range_uncommitted();
    }



    bool Volume::total_region_is_free() {
        for (uint16_t i = 0; i < this->_total_region_num; ++i) {
            const auto region = this->region_by_index(0);
            if (!region->is_free()) {
                return false;
            }
        }
        return true;
    }


    void Volume::print_on(CharOStream *out) {
        assert_lock_strong(Metaspace_lock);
        out->print(LOG_FMT, LOG_FMT_ARGS);
        out->print("reserved=");
        out->print_human_bytes(this->reserved_bytes());
        out->print(",committed=");
        out->print_human_bytes(this->committed_bytes());
        out->print(",used=");
        out->print_human_bytes(this->used_bytes());
        out->cr();
        /**
         * 打印根区域的信息
         */

        for (uint16_t i = 0; i < this->_total_region_num; ++i) {
            out->print("%2d:", i);
            this->region_by_index(i)->print_on(out);
        }
        /**
         * 打印内存内存提交情况
         */
        this->_commit_mask.print_on(out);
    }
#ifdef DIAGNOSE
    void Volume::verify() const {
        assert_lock_strong(Metaspace_lock);
        assert(this->_total_region_num >= this->_next_region_index,"check");
        assert_is_aligned((size_t)this->_reserved.start(),
                                  this->_reserved.capacity_bytes());
        assert_is_aligned(this->reserved_bytes(),RegionBytes);
        assert_is_aligned(RegionBytes,CommitGranuleBytes);
        for (uint8_t i = 0; i < this->_total_region_num; ++i) {
            this->_region[i].verify();
        }
    }
#endif

}
