//
// Created by aurora on 2022/12/16.
//
#include "Volume.hpp"
#include "Segment.hpp"
#include "CommittedBitMap.hpp"
#include "metaspace/InternalStats.hpp"
#include "meta_log.hpp"
#include "platform/os.hpp"
#include "Setting.hpp"
#include "platform/utils/align.hpp"
#include "ContextHolder.hpp"
#include "RootArea.hpp"
#include "CommittedLimiter.hpp"
#define LOG_FMT "Volume @" PTR_FORMAT " base=" PTR_FORMAT" "
#define LOG_FMT_ARGS this,this->_reserved.start()

namespace metaspace {
    Volume::Volume(Space &virtual_space,
                   std::atomic<size_t> *committed_statistics) :
            _next(nullptr),
            _reserved(virtual_space),
            _top_area(0),
            _total_area(0),
            _committed_statistics(committed_statistics),
            _commit_bitmap(virtual_space.start(), virtual_space.capacity_bytes()) {
        assert_is_aligned(virtual_space.capacity_bytes(), Setting::RegionBytes);
        this->_area_first_segment = NEW_CHEAP_ARRAY(Segment *,virtual_space.capacity_bytes() / Setting::RegionBytes ,MEMFLAG::Metaspace);
        //新增统计信息
        InternalStats::inc_num_volumes_births();
        meta_log2(debug, "出生(born),size " SIZE_FORMAT " K", this->reserved_bytes() / K);
    }

    Volume::~Volume() {

        auto reserved_bytes = this->reserved_bytes();
        meta_log2(debug, "死亡(dies),size " SIZE_FORMAT " K", reserved_bytes / K);
        auto committed_bytes = this->committed_bytes_slow_path();
        FREE_CHEAP_ARRAY(this->_area_first_segment,  MEMFLAG::Metaspace);
        /**
          * 修改虚拟链表 内存提交的统计信息
          */
        this->_committed_statistics->fetch_sub(committed_bytes);
        /**
         * 修改内部运行状态的统计信息 用户检测
         */
        InternalStats::inc_num_volumes_deaths();
    }

    void Volume::print_on(CharOStream *out) {
        assert_lock_strong(ContextHolder::global_locker());
        out->print(LOG_FMT, LOG_FMT_ARGS);
        out->print("reserved=");
        out->print_human_bytes(this->reserved_bytes());
        out->print(",committed=");
        out->print_human_bytes(this->committed_bytes_slow_path());
        out->print(",used=");

        /**
         * 打印内存内存提交情况
         */
        this->_commit_bitmap.print_on(out);
    }

    void Volume::uncommit_range(void *base, size_t bytes) {
        /**
         * 首先校验要提交区间的首地址和区间大小
         * 必须都要和内存的提交粒度对齐
         */
        const auto commit_granule = Setting::CommitGranuleBytes;
        assert_is_aligned((size_t) base, commit_granule);
        assert_is_aligned(bytes, commit_granule);

        //首先计算这个范围内提交的内存有多大
        const auto committed_bytes_in_range = this->_commit_bitmap.
                get_committed_bytes_in_range(base, bytes);
        assert_is_aligned(committed_bytes_in_range,
                          commit_granule);
        if (committed_bytes_in_range == 0) {
            /**
             * 说明之前已经完全撤销提交了 我们无需进行任何操作
             */
            meta_log2(debug, "already uncommitted:[" PTR_FORMAT "," PTR_FORMAT "),"
                    SIZE_FORMAT "K.",
                      base, (void *) ((uintptr_t) base + bytes), bytes / K);
            return;
        }
        /**
         * 下面开始实际上的取消提交
         */
        if (!os::uncommit_memory(MEMFLAG::Metaspace, base, bytes)) {
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
                  base,
                  (void *) ((uintptr_t) base + bytes),
                  bytes / K,
                  committed_bytes_in_range / K);
        /**
         * 修改虚拟节点链表 内存提交的统计信息
         */
        this->_committed_statistics->fetch_sub(committed_bytes_in_range);
        //更新统计区间
        this->_commit_bitmap.mark_range_as_uncommitted(base, bytes);
        //更新性能信息统计
        InternalStats::inc_num_range_uncommitted();
    }
    bool Volume::commit_range(void *base, size_t bytes) {
        const auto commit_granule = Setting::CommitGranuleBytes;
        /**
         * 首先校验要提交区间的首地址和区间大小
         * 必须都要和内存的提交粒度对齐
         */
        assert_is_aligned((size_t) base, commit_granule);
        assert(bytes > 0 && is_aligned(bytes, commit_granule),
               "提交区间大小非法");
        //首先计算这个范围内提交的内存有多大
        const auto committed_bytes_in_range = this->_commit_bitmap.
                get_committed_bytes_in_range(base, bytes);
        const auto end = (void *)((uintptr_t)base + bytes);
        //计算如果成功提交内存 我们会增加多少提交内存
        const auto committed_increase_bytes = bytes - committed_bytes_in_range;

        if (committed_increase_bytes == 0) {
            //说明之前已经完全提交了 我们无需再次提交
            meta_log2(debug, "已完全提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                    SIZE_FORMAT "K.",
                      base,end , bytes / K);
            return true;
        }
        /**
         * 需要检查是否大小内存提交限制
         */
        if (CommittedLimiter::_allow_expand_func()) {
            meta_log2(debug, "!!达到限制!!无法提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                    SIZE_FORMAT "K.应增加 " SIZE_FORMAT "K.",
                      base, end, bytes / K,
                      committed_increase_bytes / K);
            return false;
        }
        /**
         * 进行内存的实际提交
         */
        if (!os::commit_memory(MEMFLAG::Metaspace,base,bytes,os::CommitType::rwx)) {
            vm_exit_out_of_memory(VMErrorType::OOM_MMAP_ERROR,
                                  bytes,
                                  "为元空间(metaspace)提交内存失败");
        }
        if (global::AlwaysPreTouch) {
            os::pretouch_memory(base,  bytes);
        }
        meta_log2(debug, "提交:[" PTR_FORMAT "," PTR_FORMAT "),"
                SIZE_FORMAT "K.实际增加" SIZE_FORMAT "K.",
                  base, end,
                  bytes / K, committed_increase_bytes / K);
        CommittedLimiter::_committed_bytes.fetch_add(committed_increase_bytes);
        /**
         * 修改虚拟节点链表 内存提交的统计信息
         */
        this->_committed_statistics->fetch_add(committed_increase_bytes);
        //修改统计区间的信息
        this->_commit_bitmap.mark_range_as_committed(base, bytes);
        /**
         * 最后成功的话 增加统计信息
         */
        InternalStats::inc_num_range_committed();
        return true;
    }

    Segment *Volume::allocate_root_segment() {
        if (this->_top_area >= this->_total_area){
            return nullptr;
        }
        const auto index = this->_top_area ++;
        assert(this->_area_first_segment[index] == nullptr,"must be null");
        auto offset_start = index * Setting::RegionBytes;

    }
#ifdef DEBUG_MODE_ONLY

    void Volume::verify() const {
        assert_lock_strong(ContextHolder::global_locker());
        assert_is_aligned((size_t) this->_reserved.start(),
                          this->_reserved.capacity_bytes());
        assert_is_aligned(this->reserved_bytes(), Setting::RegionBytes);
    }




#endif

}
