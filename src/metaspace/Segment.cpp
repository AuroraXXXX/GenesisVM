//
// Created by aurora on 2022/12/16.
//

#include "Segment.hpp"
#include "metaspace/Metaspace.hpp"
#include "Volume.hpp"
#include "platform/stream/CharOStream.hpp"
#include "meta_log.hpp"

#define SEGMENT_FORMAT               \
    "Segment@" PTR_FORMAT   ","  SEGMENT_LV_FORMAT "%c,base " PTR_FORMAT

#define SEGMENT_FORMAT_ARGS(segment)   \
    segment,(segment)->_level,(segment)->get_state_char(),(segment)->_base

#define SEGMENT_FULL_FORMAT         \
    SEGMENT_FORMAT "(" SIZE_FORMAT " byte),used:" SIZE_FORMAT " byte,committed:" \
    SIZE_FORMAT  " byte,committed-free:" SIZE_FORMAT " byte"

#define SEGMENT_FULL_FORMAT_ARGS(segment)     \
    SEGMENT_FORMAT_ARGS(segment),(segment)->total_bytes(), \
    (segment)->used_bytes(),(segment)->committed_bytes(),     \
    (segment)->free_below_committed_bytes()

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
            _link_node(),
            _buddy_link_node(),
            _container(nullptr),
            _state(State::Dead),
            _level(SegmentLevel::LV_INVALID),
            _committed_bytes(0),
            _used_bytes(0),
            _base(0) {

    }

    void Segment::clear() {
        this->_base = 0;
        this->_committed_bytes = this->_used_bytes = 0;
        this->_level = SegmentLevel::LV_INVALID;
        this->_container = nullptr;
        this->_state = State::Dead;
    }

    void *Segment::allocate(size_t request_bytes) {
        assert(this->free_below_committed_bytes() >= request_bytes,
               "未确保当前已分配内存中空闲内存" SIZE_FORMAT"，可以满足用户需求" SIZE_FORMAT,
               this->free_below_committed_bytes(), request_bytes);
        auto used_top = this->_base + this->_used_bytes;
        this->_used_bytes += request_bytes;
        return (void *) used_top;
    }


    bool Segment::commit_up_to(size_t new_commit_bytes) {
        assert_lock_strong(Metaspace::locker());
        assert(is_clamp(new_commit_bytes,this->committed_bytes(),), "无法缩小提交内存边界");
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
        const auto commit_granule = Setting::CommitGranuleBytes;
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
        /**
         * 确保[base,base + bytes)这个区间内存被提交
         * 若这个区间小于提交粒度 会向两侧对齐 满足提交粒度的大小
         * 然后进行提交 若这个内存粒度已经被提交完毕 那么不会有任何影响
         *
         * 若这个区间大于是提交粒度的N倍 那么由于不与其他内存块共享粒度
         * 对于其他内存块不会有影响
         */
        uintptr_t range_base = align_down(this->_base + commit_from, commit_granule);
        uintptr_t range_end = align_up(this->_base + commit_to, commit_granule);
//        const auto commit_result = this->container()->commit_range((void *) range_base, range_end - range_base);
//        if (commit_result)
//            this->_committed_bytes = commit_to;
//        return commit_result;
        return false;
    }


    bool Segment::ensure_committed_enough(size_t bytes) {
        bool result = true;
        assert(this->free_bytes() >= bytes, "溢出");
        if (bytes >= this->free_below_committed_bytes()) {

                result = this->commit_up_to(bytes);

        }
        return result;
    }

    void Segment::uncommit() {
        assert_lock_strong(Metaspace::locker());
        assert(this->_state == State::Free &&
               this->used_bytes() == 0 &&
               this->total_bytes() >= Setting::CommitGranuleBytes,
               "仅仅空闲块且尺寸大于提交粒度才允许撤销提交");
        const auto total_bytes = this->total_bytes();
        if (total_bytes >= Setting::CommitGranuleBytes) {
//            this->container()->uncommit_range((void*)this->_base, total_bytes);
            this->_committed_bytes = 0;
        }
    }

    void Segment::print_on(CharOStream *out) const {
        out->print(SEGMENT_FULL_FORMAT, SEGMENT_FULL_FORMAT_ARGS(this));
    }


}
