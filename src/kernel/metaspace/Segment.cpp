//
// Created by aurora on 2022/12/16.
//

#include "Segment.hpp"
#include "kernel/metaspace/constants.hpp"
#include "Volume.hpp"
#include "plat/stream/CharOStream.hpp"
#include "meta_log.hpp"
#include "kernel_mutex.hpp"
namespace metaspace {
    char Segment::get_state_char() const {
        switch (this->_state) {
            case State::InUse:
                return 'U';
            case State::Free:
                return 'F';
            case State::Dead:
                return 'D';
            default:
                return '?';
        }
    }

    Segment::Segment() :
            _state(State::Dead),
            _level(SegmentLevel::LV_ROOT),
            _committed_bytes(0),
            _used_bytes(0),
            _base(0),
            SegmentBase<Segment>() {

    }

    void Segment::clear() {
        this->_base = 0;
        this->_committed_bytes = this->_used_bytes = 0;
        this->_level = SegmentLevel::LV_ROOT;
    }

    void *Segment::allocate(size_t request_bytes) {
        assert(this->free_below_committed_bytes() >= request_bytes,
               "未确保当前已分配内存中空闲内存" SIZE_FORMAT"，可以满足用户需求" SIZE_FORMAT,
               this->free_below_committed_bytes(), request_bytes);
        auto used_top = this->used_top();
        this->_used_bytes += request_bytes;
        return used_top;

    }

    void Segment::initialize(Volume *container, void *base, SegmentLevel level) {
        this->_used_bytes = this->_committed_bytes = 0;
        this->_base = (uintptr_t)base;
        this->_level = level;
        this->set_container(container);
    }

    bool Segment::commit_up_to(size_t new_commit_bytes) {
        assert_lock_strong(Metaspace_lock);
        assert(new_commit_bytes > this->committed_bytes(), "无法缩小提交内存边界");
        /**
         * 在包含提交部分和未提交区间调用VirtualSpace::commit_range时，
         * 会将现有内容擦除，因此我们需要确保 我们不会在活动数据范围内调用
         *
         * 1 在大于或者等于提交粒度块时，由于块的几何结构，块会覆盖整个提交粒度
         * 那么由于我们独占这个提交颗粒，在这个块中，无论我们提交还是撤销提交
         * 都不会影响其他块 。只要我们块本身不要重复提交即可
         * 在本身 我们使用committed_top指针来限制
         *
         * 2 在比提交粒度小的内存块时 我们会与伙伴块共享一个提交粒度
         * 此时 存在两种情况
         * -- 要么提交粒度完全提交,此时当前块和伙伴块都包含活动数据
         * 在这种情况下 VirtualSpace::commit_range 什么事情都不会做
         * -- 要么提交粒度完全未提交 那么当前块和伙伴块都不包含活动数据
         * 即伙伴块也未提交 我们整个时候提交即可
         */
        /**
         * 旧的内存提交大小
         */
        const auto commit_from = this->committed_bytes();
        const auto commit_granule = CommitGranuleBytes;
        /**
         * 将新的提交边界进行对齐 并且进行最大约束
         * 得到我们希望的新的提交边界
         */
        const auto commit_to = MIN2(align_up(new_commit_bytes, commit_granule),
                this->total_bytes());
        assert(commit_from >= this->used_bytes(), "健全");
        assert(commit_to <= this->total_bytes(), "健全");
        log_debug(metaspace)(SEGMENT_FORMAT ":尝试将已提交内存:" SIZE_FORMAT
                             " bytes => " SIZE_FORMAT " bytes",
                             SEGMENT_FORMAT_ARGS(this), commit_from, commit_to);
        auto res =  this->ensure_range_is_committed((void *)(this->_base + commit_from),
                                               commit_to - commit_from);
        if(res){
            this->set_committed_bytes(commit_to);
        }
        return true;
    }

    bool Segment::ensure_committed_enough_and_acquire_lock(size_t bytes) {
        bool result = true;
        assert(this->free_bytes() >= bytes, "溢出");
        if (bytes >= this->free_below_committed_bytes()) {
            MutexLocker fcl(Metaspace_lock);
            result = this->commit_up_to(bytes);
        }
        return result;
    }

    void Segment::uncommit() {
        assert_lock_strong(Metaspace_lock);
        assert(this->is_free() &&
               this->used_bytes() == 0 &&
               this->total_bytes() >= CommitGranuleBytes,
               "仅仅空闲块且尺寸大于提交粒度才允许撤销提交");
        const auto total_bytes = this->total_bytes();
        if (total_bytes >= CommitGranuleBytes) {
            this->container()->uncommit_range(this->base(), total_bytes);
            this->_committed_bytes = 0;
        }
    }

    void Segment::print_on(CharOStream *out) const {
        out->print(SEGMENT_FULL_FORMAT, SEGMENT_FULL_FORMAT_ARGS(this));
    }

    bool Segment::ensure_committed_enough(size_t bytes) {
        bool result = true;
        assert(this->free_bytes() >= bytes, "溢出");
        assert_lock_strong(Metaspace_lock);
        if (bytes >= this->free_below_committed_bytes()) {
            result = this->commit_up_to(bytes);
        }
        return result;
    }

    bool Segment::ensure_range_is_committed( void* base, size_t bytes) {
        assert_lock_strong(Metaspace_lock);
        assert(base && bytes > 0, "健全");
        auto commit_granule = CommitGranuleBytes;
        uintptr_t range_base = align_down((size_t)base,commit_granule);
        uintptr_t range_end = align_up((size_t)base + bytes,commit_granule);
        assert(bytes > 0 && is_aligned(bytes,commit_granule),"内存大小错误");
        return this->container()->commit_range((void *)range_base,range_end - range_base);
    }


}
