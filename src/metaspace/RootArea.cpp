//
// Created by aurora on 2025/3/25.
//

#include "RootArea.hpp"
#include "new"
#include "platform/allocation.hpp"
#include "Segment.hpp"
#include "Volume.hpp"
#include "platform/mem/ResourceArenaMark.hpp"
#include "meta_log.hpp"
#include "LevelSegmentArray.hpp"
#include "SegmentHeaderPool.hpp"

metaspace::RootArea::RootArea(metaspace::Volume *volume, uint32_t total) :
        _top(0),
        _total(total),
        _volume(volume) {

}

metaspace::RootArea *metaspace::RootArea::create(metaspace::Volume *volume, uint32_t len) {
    const size_t total_bytes = sizeof(RootArea) + sizeof(LinkList<Segment>) * len;
    const auto mem_ptr = NEW_CHEAP_ARRAY(char, total_bytes, MEMFLAG::Metaspace);
    return new(mem_ptr)RootArea(volume, len);
}

metaspace::Segment *metaspace::RootArea::merge(metaspace::Segment *segment, metaspace::LevelSegmentArray *array) {
    assert(!segment->is_root_segment(), "root segment is not merge again");
    assert(segment->_state == Segment::State::Free && segment->_used_bytes == 0, "segment must be free");

    ResourceArenaMark mark;
    Segment *result_segment = nullptr;
    do {
        const bool is_leader = segment->is_leader();
        const auto buddy_node = &segment->_buddy_link_node;
        //获取其伙伴的地址
        const auto buddy = is_leader ? buddy_node->next() : buddy_node->prev();
        /**
         * 按照切割的算法
         * 我们伙伴块的内存大小 一定是小于或者等于 当前块
         * 即伙伴块等级大于或者等于当前块的
         */
        assert(buddy->level() >= segment->level(), "健全");
        if (buddy->level() != segment->level() || buddy->_state != Segment::State::Free) {
            //只要伙伴块与原内存块等级不相同或者伙伴块只要不是空闲的 那么无法合并
            meta_log_stream(trace);
            log.print("buddy segment cannot merge,buddy:");
            segment->print_on(&log);
            break;
        }
        {
            meta_log_stream(trace);
            log.print("buddy segment merge,buddy:");
            segment->print_on(&log);
        }
        //从空闲块管理器中移除 伙伴块
        assert(buddy->_state == Segment::State::Free, "程序错误");
        array->remove(buddy);

        //确定当前块的领导者和跟随者
        Segment *leader, *follower;
        if (is_leader) {
            leader = segment;
            follower = buddy;
        } else {
            leader = buddy;
            follower = segment;
        }

        /**
         * 这里进行断言 看看我们代码写的是否正确
         * 领导者和跟随者的内存块等级应该相同
         * 且虚拟地址空间连接在一起
         * 并且二者都是空闲的
         */
        assert(leader->_base + leader->total_bytes() == follower->_base &&
               leader->_level == follower->_level &&
               leader->_state == Segment::State::Free &&
               follower->_state == Segment::State::Free, "check");

        /**
         * 统计 合并后的内存块的提交内存大小
         * 只有领导者的内存块完全提交 我们才会将跟随者的提交内存计算在内
         * 这样提交内存才不会出现漏洞 出现意想不到的错误
         */
        size_t merged_committed_bytes = leader->committed_bytes();
        if (merged_committed_bytes == leader->total_bytes()) {
            merged_committed_bytes += follower->committed_bytes();
        }

        /**
         * 删除follower节点
         */
        {
            auto leader_node = &leader->_buddy_link_node;
            auto follower_node = &follower->_buddy_link_node;
            auto follower_next = follower_node->next();
            leader_node->set_next(follower_next);
            if (follower_next != nullptr) {
                auto follower_next_node = &follower_next->_buddy_link_node;
                follower_next_node->set_prev(leader);
            }
            follower_node->clear();
        }

        //合并后 将跟随者的内存块头部放入池中
        segment->_state = Segment::State::Dead;
        SegmentHeaderPool::pool()->free(follower);
        /**
         * 合并完成后 调整领导者的 提交内存和内存等级
         */
        --leader->_level;
        leader->_committed_bytes = merged_committed_bytes;
        //进行中止条件的判断
        if (leader->is_root_segment()) {
            break;
        }
        //进行下一次循环
        result_segment = segment = leader;
    } while (true);
    return result_segment;
}

metaspace::RootArea::~RootArea() {
    for (uint32_t i = 0; i < this->_top; ++i) {
        auto segment = this->_segments[i];
        assert(segment->is_root_segment() && segment->_state == Segment::State::Free, "must be");
        segment->_state = Segment::State::Dead;
        SegmentHeaderPool::pool()->free(segment);
    }
}

void metaspace::RootArea::split(metaspace::Segment *const source_segment,
                                metaspace::SegmentLevel_t target_level,
                                metaspace::LevelSegmentArray *array) {
    assert(source_segment != nullptr, "源块(Segment)不可以为空");
    assert(source_segment->state() == Segment::State::Free, "source segment must be free");
    //目标块的大小一定小于源块的大小 否则无法从元块上进行切割
    assert(target_level > source_segment->level(), "source segment level must be greater than target level");
    //levelA < levelB 表示 内存块A大小 大于 内存块B大小 我们还需进行切割才能满足需求
    while (source_segment->level() > target_level) {
        {
            ResourceArenaMark mark;
            meta_log_stream(trace);
            log.print("split segment:");
            source_segment->print_on(&log);
        }
        //记录旧的segment提交的内存
        const auto old_committed_bytes = source_segment->committed_bytes();
        //下面将源segment切分
        --source_segment->_level;
        //管理 切分后的segment 的虚拟地址空间
        const auto split_segment = SegmentHeaderPool::pool()->alloc();
        // 标记状态是空闲的
        split_segment->_state = Segment::State::Free;
        {

            //将管理内存添加进去
            split_segment->_base = source_segment->_base +  source_segment->total_bytes();
            split_segment->_level = source_segment->_level;
            split_segment->_container = source_segment->_container;
            split_segment->_committed_bytes = 0;
        }
        {
            //调整source_segment与split_segment的提交内存
            auto source_segment_bytes = source_segment->total_bytes();
            if (old_committed_bytes >= source_segment_bytes){
                //说明 已经提交的内存大于被一分为二的内存块 我们要分别设置已提交内存大小
                source_segment->_committed_bytes = source_segment_bytes ;
                split_segment->_committed_bytes = old_committed_bytes - source_segment_bytes;
            }else{
                //没有大于一半  那说明另外一块没有已提交内存
                split_segment->_committed_bytes = 0;
            }
        }

        {
            /**
            * 最后调整用于在内存块在虚拟节点中前驱和后继关系
            * 由原来的:
            * source_segment <---> next_segment
            * 变成:
            * source_segment <---> splinter_segment <---> next_segment
            */
            auto source_segment_node = &source_segment->_buddy_link_node;
            auto next_segment = source_segment_node->next();
            auto next_segment_node = &next_segment->_buddy_link_node;
            auto splinter_segment_node = &split_segment->_buddy_link_node;
            source_segment_node->set_next(split_segment);
            splinter_segment_node->set_next(next_segment);
            next_segment_node->set_prev(split_segment);
            splinter_segment_node->set_prev(source_segment);
        }
        {
            ResourceArenaMark mark;
            meta_log_stream(trace);
            log.print("...result segment:");
            source_segment->print_on(&log);
            log.print("...split segment:");
            split_segment->print_on(&log);
        }
        array->add(split_segment);
    }

}

metaspace::Segment *metaspace::RootArea::alloc_root_segment() {
    Segment* segment = nullptr;
    if (this->_top < this->_total) {
        auto current_index = this->_top ++;
        segment = SegmentHeaderPool::pool()->alloc();
        segment->_state = Segment::State::Free;
        segment->_level = SegmentLevel::LV_ROOT;
        segment->_container = this->_volume;
        segment->_committed_bytes = 0;
        segment->_base = this->_volume->_reserved.start_literal() + current_index * Setting::RegionBytes;
        this->_segments[current_index] = segment;
    }
    return segment;
}

