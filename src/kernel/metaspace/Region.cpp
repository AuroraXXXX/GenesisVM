//
// Created by aurora on 2022/12/23.
//

#include "Region.hpp"
#include "meta_log.hpp"
#include "Segment.hpp"
#include "SegmentManager.hpp"
#include "SegmentHeaderPool.hpp"
#include "kernel/metaspace/constants.hpp"

#define LOG_FMT "Region @" PTR_FORMAT " base=" PTR_FORMAT" "
#define LOG_FMT_ARGS this,this->_base
namespace metaspace {

    Segment *Region::merge(Segment *segment,
                           SegmentManager *manager) const {
        DEBUG_MODE_ONLY(assert(this->contain(segment->base()), "region is not contain this segment");)
        assert(!segment->is_root_segment(), "无法再次合并了");
        assert(segment->is_free() && segment->used_bytes() == 0, "仅可以合并空闲的内存块(Segment)");
        meta_log2(trace, "尝试合并块" SEGMENT_FORMAT ".", SEGMENT_FORMAT_ARGS(segment));
        Segment *result_segment = nullptr;
        do {
            const bool is_leader = segment->is_leader();
            //获取其伙伴的地址
            const auto buddy = is_leader ? segment->next_buddy() : segment->prev_buddy();
            /**
             * 按照切割的算法
             * 我们伙伴块的内存大小 一定是小于或者等于 当前块
             * 即伙伴块等级大于或者等于当前块的
             */
            assert(buddy->level() >= segment->level(), "健全");
            if (buddy->level() != segment->level() || !buddy->is_free()) {
                //只要伙伴块与原内存块等级不相同或者伙伴块只要不是空闲的 那么无法合并
                meta_log2(trace, "无法使用伙伴块合并:" SEGMENT_FORMAT,
                          SEGMENT_FORMAT_ARGS(buddy));
                break;
            }
            meta_log2(trace, "将使用伙伴块合并:" SEGMENT_FORMAT,
                      SEGMENT_FORMAT_ARGS(buddy));

            //从空闲块管理器中移除 伙伴块
            assert(buddy->is_free(), "程序错误");
            manager->remove(buddy);

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
            assert(leader->end() == follower->base() &&
                   leader->level() == follower->level() &&
                   leader->is_free() && follower->is_free(), "健全");
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
             * 调整虚拟节点中伙伴关系
             */
            leader->set_next_buddy(follower->next_buddy());
            if (follower->next_buddy()) {
                follower->next_buddy()->set_prev_buddy(leader);
            }
            //合并后 将跟随者的内存块头部放入池中
            SegmentHeaderPool::pool()->deallocate_segment_header(follower);
            /**
             * 合并完成后 调整领导者的 提交内存和内存等级
             */
            leader->dec_level();
            leader->set_committed_bytes(merged_committed_bytes);
            //进行中止条件的判断
            if (leader->is_root_segment()) {
                break;
            }
            //进行下一次循环
            result_segment = segment = leader;
        } while (true);
        return result_segment;
    }

    void Region::split(SegmentLevel target_level,
                       Segment *source_segment,
                       SegmentManager *manager) const {
        DEBUG_MODE_ONLY(assert(this->contain(source_segment->base()),
                               "region is not contain this segment");)

        assert(source_segment != nullptr, "源块(Segment)不可以为空");
        assert(source_segment->is_free(), "源块(Segment)必须是空闲状态,才可以进行分割");
        //目标块的大小一定小于源块的大小 否则无法从元块上进行切割
        assert(target_level > source_segment->level(), "目标内存块(Segment)的等级错误");
        //levelA < levelB 表示 内存块A大小 大于 内存块B大小 我们还需进行切割才能满足需求
        while (source_segment->level() < target_level) {
            meta_log2(trace, "正在切割块: " SEGMENT_FULL_FORMAT ".",
                      SEGMENT_FULL_FORMAT_ARGS(source_segment));
            //统计旧的内存块已经提交的大小
            const size_t old_committed_bytes = source_segment->committed_bytes();
            /**
             * 先表明内存块缩小2倍 那么内存块的结束指针和长度会发生改变
             */
            source_segment->inc_level();
            //需要设置被分割出来的分裂块
            const auto splinter_segment = SegmentHeaderPool::pool()->
                    allocate_segment_header();
            //由于之前已经缩小2倍 那么现在的结束地址就是新的跟随内存块的首地址
            splinter_segment->initialize(source_segment->container(),
                                         source_segment->end(),
                                         source_segment->level());
            //调整分裂后内存块的已提交内存大小
            {
                auto splinted_segment_size = source_segment->total_bytes();
                if (old_committed_bytes >= splinted_segment_size) {
                    //说明 已经提交的内存大于被一分为二的内存块 我们要分别设置已提交内存大小
                    source_segment->set_committed_bytes(splinted_segment_size);
                    splinter_segment->set_committed_bytes(old_committed_bytes -
                                                          splinted_segment_size);
                } else {
                    //没有大于一半  那说明另外一块没有已提交内存
                    splinter_segment->set_committed_bytes(0);
                }
            }
            /**
             * 最后调整用于在内存块在虚拟节点中前驱和后继关系
             * 由原来的:
             * source_segment <---> next_segment
             * 变成:
             * source_segment <---> splinter_segment <---> next_segment
             */
            {
                const auto next_segment = source_segment->next_buddy();
                if (next_segment) {
                    next_segment->set_prev_buddy(splinter_segment);
                }
                splinter_segment->set_next_buddy(next_segment);
                splinter_segment->set_prev_buddy(source_segment);
                source_segment->set_next_buddy(splinter_segment);
            }
            meta_log2(trace, "..结果块:" SEGMENT_FULL_FORMAT,
                      SEGMENT_FULL_FORMAT_ARGS(source_segment));
            meta_log2(trace, "..分裂块:" SEGMENT_FULL_FORMAT,
                      SEGMENT_FULL_FORMAT_ARGS(splinter_segment));
            //切割好了 那么就把跟随者内存块放入管理器中管理
            manager->add(splinter_segment);
        }
        assert(target_level == source_segment->level(), "程序错误");
    }

    Region::~Region() {
        /**
         * 当VirtualSpaceNode被销毁(清除)时调用。当然，所有的块都应该是空闲的。
         * 事实上，应该只有一个数据块，
         * 因为所有的空闲数据块都应该被合并。
         */
        if (this->_first) {
            assert(_first->is_root_segment() && _first->is_free(),
                   "如果不是所有的块都是空闲的，则不能删除根块区域(RootChunk).");
            SegmentHeaderPool::pool()->deallocate_segment_header(this->_first);
            this->_first = nullptr;
        }
    }

    bool Region::is_free() {
        //没有设置第一个内存块 或者 当前已经是根块还是空闲的
        return this->_first == nullptr ||
               (this->_first->is_root_segment() &&
                this->_first->is_free());
    }

    Segment *Region::alloc_root_segment(Volume *container) {
        assert(this->_first == nullptr, "已经存在一个根块了");
        auto segment = SegmentHeaderPool::pool()->allocate_segment_header();
        segment->initialize(container, this->base(), SegmentLevel::LV_ROOT);
        this->_first = segment;
        return segment;
    }

    bool Region::attempt_enlarge_segment(Segment *segment, SegmentManager *manager) const {
        DEBUG_MODE_ONLY(assert(this->contain(segment->base()),
                               "region is not contain this segment");)
        assert(!segment->is_root_segment(), "根块无法再扩展了");
        assert(segment->is_inuse(), "仅可扩展正在使用中的内存块");
        if (!segment->is_leader()) {
            //不是领导者 无法扩展
            return false;
        }
        const auto buddy = segment->next_buddy();
        //伙伴块当然没有自身大 不明白请看切割算法
        assert(buddy->level() >= segment->level(), "健全");
        if (!buddy->is_free()) {
            return false;
        }
        //仅仅在伙伴块和当前块大小相同 才可以进行合并
        if (buddy->level() != segment->level()) {
            return false;
        }
        //下面开始合并
        meta_log2(trace, "正在扩展块" SEGMENT_FULL_FORMAT
                ",通过伙伴块" SEGMENT_FULL_FORMAT,
                  SEGMENT_FULL_FORMAT_ARGS(segment),
                  SEGMENT_FULL_FORMAT_ARGS(buddy));
        //下面 统计合并后块的 提交内存大小
        auto merged_committed_bytes = segment->committed_bytes();
        /**
         * 原扩展块的所有空间全部被提交了 那么就可以和伙伴块的提交内存 连在一起
         * 否则提交内存会出现破洞
         */
        if (merged_committed_bytes == segment->total_bytes()) {
            merged_committed_bytes += segment->committed_bytes();
        }
        //将伙伴块从伙伴关系链表中移除
        auto next = buddy->next_buddy();
        if (next) {
            next->set_prev_buddy(segment);
        }
        segment->set_next_buddy(next);
        //将伙伴块从空闲管理器中移除
        manager->remove(buddy);
        //将节点的头归还
        SegmentHeaderPool::pool()->deallocate_segment_header(buddy);
        //修改合并后块信息
        segment->dec_level();
        segment->set_committed_bytes(merged_committed_bytes);
        meta_log2(debug, "已扩展块 " SEGMENT_FULL_FORMAT,
                  SEGMENT_FULL_FORMAT_ARGS(segment));
        return true;
    }

    void Region::print_on(CharOStream *out) const {
        out->print(PTR_FORMAT ": ", this->base());
        auto segment = this->_first;
        if (segment == nullptr) {
            out->print_cr(" (无内存块)");
            return;
        }
        //下面说明存在内存块
        while (segment) {
            auto lev = segment->level();
            if (!level_is_valid(lev)) {
                out->print("??? ");
            } else {
                out->print(SEGMENT_LV_FORMAT "%c ", lev, segment->get_state_char());
            }
            segment = segment->next_buddy();
        }
        out->cr();
    }

#ifdef DIAGNOSE

    void Region::verify() const {
        auto cur = this->_first;
        while (cur != nullptr) {
            auto prev_buddy = cur->prev_buddy();
            auto next_buddy = cur->next_buddy();

            assert(prev_buddy == nullptr || prev_buddy->next_buddy() == cur,
                   "check");
            assert(next_buddy == nullptr || next_buddy->prev_buddy() == cur,
                   "check");
            auto buddy = cur->is_leader() ? cur->next_buddy() : cur->prev_buddy();
            assert(buddy->level() >= cur->level(), "check");
            assert(is_clamp<>(cur->level(), SegmentLevel::LV_LOWEST, SegmentLevel::LV_HIGHEST), "check");
            assert(cur->is_free() || cur->is_inuse(), "error status");
            auto reserved_bytes = cur->total_bytes();
            assert(cur->committed_bytes() <= reserved_bytes, "check");
            assert(cur->used_bytes() <= cur->committed_bytes(), "check");
            cur = cur->next_buddy();
        }

    }

#endif

}
