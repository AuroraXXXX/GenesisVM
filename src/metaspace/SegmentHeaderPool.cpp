//
// Created by aurora on 2022/12/23.
//

#include "SegmentHeaderPool.hpp"
namespace metaspace {
    SegmentHeaderPool* SegmentHeaderPool::_pool = nullptr;
    SegmentHeaderPool::SegmentHeaderPool() :
            _slab_top(0),
            _dead_headers(),
            _slab_list(){

    }

    SegmentHeaderPool::~SegmentHeaderPool() {
        while (!this->_slab_list.is_empty()) {
            //弹出元素
           auto ele = this->_slab_list.pop();
           delete ele;
        }
    }

    void SegmentHeaderPool::allocate_new_slab() {
        auto slab = new Slab();
        this->_slab_list.push(slab);
        this->_slab_top = 0;
    }

    Segment *SegmentHeaderPool::alloc() {
        /**
         * 首先从_dead_segment中获取
         */
        Segment *head = this->_dead_headers.pop_head();
        if (head == nullptr) {
            //没有现成可用的Dead内存块 那么就需要从Slab中申请
            if (this->_slab_list.is_empty() ||
                this->_slab_top == SlabCapacity) {
                //如果当前没有Slab或者已经使用完毕了 那么就需要申请一个新的
                this->allocate_new_slab();
                assert(this->_slab_top == 0,"check");
            }
            //申请 并调整ID
            auto first_slab = this->_slab_list.peek();
            const auto current_index = this->_slab_top.fetch_add(1);
            head = first_slab->_elems + current_index;
        }
        assert(head != nullptr && head->state() == Segment::State::Dead,"check");
        this->_used_headers.fetch_add(1);
        return head;
    }

    void SegmentHeaderPool::free(Segment *segment) {
        assert(segment!= nullptr && segment->state() == Segment::State::Free,"错误");
        assert(segment->used_bytes() == 0,"must be");
        this->_dead_headers.push_head(segment);
        this->_used_headers.fetch_sub(1);
    }

    void SegmentHeaderPool::initialize() {
        assert(SegmentHeaderPool::_pool == nullptr, "ChunkHeaderPool仅仅可以初始化一次");
        SegmentHeaderPool::_pool = new SegmentHeaderPool();
    }
}