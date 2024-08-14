//
// Created by aurora on 2022/12/23.
//

#include "SegmentHeaderPool.hpp"
namespace metaspace {
    SegmentHeaderPool* SegmentHeaderPool::_pool = nullptr;
    SegmentHeaderPool::SegmentHeaderPool() :
            _slab_nums(0),
            _first_slab(nullptr),
            _dead_segments(),
            _dead_segments_num(0),
            _slab_top(0),
            _used_headers(0) {

    }

    SegmentHeaderPool::~SegmentHeaderPool() {
        auto slab = this->_first_slab;
        while (slab) {
            auto next = slab->_next;
            delete slab;
            slab = next;
        }
        SegmentHeaderPool::_pool = nullptr;
    }

    void SegmentHeaderPool::allocate_new_slab() {
        auto slab = new Slab();
        slab->_next = this->_first_slab;
        this->_first_slab = slab;
        ++this->_slab_nums;
        this->_slab_top = 0;
    }

    Segment *SegmentHeaderPool::allocate_segment_header() {
        /**
         * 首先从_dead_chunk中获取
         */
        Segment *chunk_head = nullptr;
        if (!this->_dead_segments.is_empty()) {
            //存在死亡的内存块
            chunk_head = this->_dead_segments.delete_from_list_head();
            --this->_dead_segments_num;
            //获取到了 但是我们需要进行数据的擦除 给与一个干净的头部
            chunk_head->clear();
        }
        assert(chunk_head == nullptr || chunk_head->is_dead(), "错误");
        if (chunk_head == nullptr) {
            //没有现成可用的Dead内存块 那么就需要从Slab中申请
            if (this->_first_slab == nullptr ||
                this->_slab_top == SlabCapacity) {
                //如果当前没有Slab或者已经使用完毕了 那么就需要申请一个新的
                this->allocate_new_slab();
                assert(this->_slab_top == 0,"错误");
            }
            //申请 并调整ID
            chunk_head = this->_first_slab->_elems + this->_slab_top;
            ++this->_slab_top;
        }
        assert(chunk_head->is_dead(),"ChunkHeader状态设置错误");
        this->_used_headers++;
        //将内存块头部的状态设置为空闲
        chunk_head->set_free();
        return chunk_head;
    }

    void SegmentHeaderPool::deallocate_segment_header(Segment *chunk) {
        assert(chunk!= nullptr&&chunk->is_free(),"错误");
        chunk->set_dead();
        this->_dead_segments.head_add_to_list(chunk);
        ++this->_dead_segments_num;
        --this->_used_headers;
    }

    void SegmentHeaderPool::initialize() {
        assert(SegmentHeaderPool::_pool == nullptr, "ChunkHeaderPool仅仅可以初始化一次");
        SegmentHeaderPool::_pool = new SegmentHeaderPool();
    }
}