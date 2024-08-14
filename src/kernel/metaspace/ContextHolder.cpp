//
// Created by aurora on 2022/12/16.
//

#include "ContextHolder.hpp"
#include "meta_log.hpp"
#include "kernel_mutex.hpp"
#include "Segment.hpp"
#include "Volume.hpp"
#include "Region.hpp"
#include "kernel/metaspace/InternalStats.hpp"

#define LOG_FMT         "ContextHolder @" PTR_FORMAT
#define LOG_FMT_ARGS    this
namespace metaspace {
    ContextHolder *ContextHolder::_context = nullptr;
    ContextHolder::ContextHolder(
            SegmentManager *segment_mgr,
            VolumeList *volume_list) :
            _segment_mgr(segment_mgr),
            _volume_list(volume_list),
            _used_bytes(0) {
        meta_log(debug, "出生(born)");
    }

    ContextHolder::~ContextHolder() {
        delete this->_segment_mgr;
        delete this->_volume_list;
    }

    void ContextHolder::return_segment(Segment *segment) {
        //获取锁 因为之后只能一个线程进入
        MutexLocker fc(Metaspace_lock);
        this->return_segment_with_lock(segment);
    }

    bool ContextHolder::attempt_enlarge_segment(Segment *segment) {
        MutexLocker fcl(Metaspace_lock);
        auto region = segment->container()->region_by_pointer(segment->base());
        bool res = region->attempt_enlarge_segment(segment, this->_segment_mgr);
        if (res) {
            //增加 扩展的统计信息
            InternalStats::inc_num_segments_enlarged();
            meta_log(debug, "已扩展Segment");
        }
        return res;
    }

    Segment *ContextHolder::search_satisfy_segment_in_free(
            SegmentLevel preferred_level,
            SegmentLevel max_level,
            size_t suggest_min_committed_bytes) {

        Segment *segment;

        /**
         * 首先寻找已提交内存满足min_committed_bytes(请求时需求的最小已提交内存)的内存块
         * 1 从preferred_level等级的内存块升序查找（即从大内存块找到小内存块）
         * 但是现在，只考虑大于某个阈值的块——这是为了防止大型加载器
         * (例如boot)不必要地吞噬lambdas留下的所有小碎片块。
         */
        const auto step_max_level = MIN2((SegmentLevel) ((SegementLevel_t)preferred_level + 2),
                                         max_level);
        segment = this->_segment_mgr->search_segment_ascending(
                preferred_level,
                step_max_level,
                suggest_min_committed_bytes);
        if (segment) {
            return segment;
        }


        /**
         * 2 在较大内存块中(内存块等级<= preferred_level) 搜寻是否有满足最小提交内存的内存块
         */
        segment = this->_segment_mgr->search_segment_descending(
                preferred_level,
                suggest_min_committed_bytes);
        if (segment) {
            return segment;
        }

        /**
         * 3 再次重复进行第一次的查找 但是这次只要求最小提交内存满足要求即可
         *  不再可虑是否可能吞噬小的内存块
         */
        segment = this->_segment_mgr->search_segment_ascending(
                preferred_level,
                max_level,
                suggest_min_committed_bytes);
        if (segment) {
            return segment;
        }
        /**
         * 4 到了这里还没有找到 那么我们只有寻找满足要求的虚拟地址空间
         *  然后进行提交
         */
        segment = this->_segment_mgr->search_segment_ascending(preferred_level,
                                                               max_level,
                                                               0);
        if (segment) {
            return segment;
        }

        /**
         * 5 满足要求的虚拟地址空间也没有找到 那么就搜寻更大的虚拟地址空间
         */
        segment = this->_segment_mgr->search_segment_descending(preferred_level,
                                                                0);
        return segment;
    }

    Segment *ContextHolder::get_segment(SegmentLevel preferred_level,
                                        SegmentLevel max_level,
                                        size_t min_committed_bytes) {
        /**
         * 首先希望的内存块等级 应该大于等于 最大的内存块等级
         * 即希望的内存块大小应大于等于至少内存块大小
         *
         * 第二的断言 要求 最小提交大小 和最大内存块等级的正确性
         */
        assert(preferred_level <= max_level, "健全");
        assert(bytes_to_level(min_committed_bytes) >= max_level, "健全");
        assert(level_is_valid(preferred_level) &&
               level_is_valid(max_level), "Segment Level错误");
        MutexLocker fcl(Metaspace_lock);
        /**
         * 日志的输出
         */
        meta_log2(debug, "请求的segment pref_level:" SEGMENT_LV_FORMAT
                ",max_level:" SEGMENT_LV_FORMAT
                ",min_committed_bytes:" SIZE_FORMAT,
                  preferred_level, max_level, min_committed_bytes);
        auto segment = this->search_satisfy_segment_in_free(
                preferred_level,
                max_level,
                min_committed_bytes);
        if (segment) {
            meta_log(trace, "已从空闲的segment中获取到segment");
        } else {
            /**
             * 到这里 说明整个空闲内存块管理器
             * 无法获取满足要求的内存块
             * 我们需要申请得到一个根块 然后进行切分
             *
             */
            segment = this->_volume_list->allocate_root_segment();
            if (segment) {
                //通知获取到了内存块
                assert(segment->is_root_segment(), "此处必须是根块");
                meta_log(debug, "已申请一个新的Root segment");
            } else {
                meta_log(info, "申请一个新的Root segment失败");
            }
        }
        if (segment == nullptr) {
            /**
             * 到这里还是失败 说明我们无法申请成功
             */
            meta_log2(info, "获取segment失败,pref_level:" SEGMENT_LV_FORMAT
                    ",max_level:" SEGMENT_LV_FORMAT,
                      preferred_level, max_level);
            return nullptr;
        }
        /**
         * 到这里说明我们获取到了一个内存块
         * 但是可能不是我们想要的 我们需要进行切割
         */
        if (segment->level() < preferred_level) {
            this->split_segment(segment, preferred_level);
        }
        /**
         * 获取了虚拟地址空间大小 那么需要进行内存的提交
         */
        if (segment->committed_bytes() < min_committed_bytes &&
            !segment->ensure_committed_enough(min_committed_bytes)) {
            //现在提交内存不足 但是提交失败了
            meta_log2(info, "在" SEGMENT_FORMAT "上提交" SIZE_FORMAT" bytes失败!",
                      SEGMENT_FORMAT_ARGS(segment), min_committed_bytes);
            //需要把这个内存块提交到 空闲链表中
            this->return_segment_with_lock(segment);
            return nullptr;
        }
        /**
         * 我们获得了符合要求的内存块
         */
        assert(segment->committed_bytes() >= min_committed_bytes, "健全");
        assert(segment->used_bytes() == 0, "健全");
        //设置块的状态
        segment->set_inuse();
        meta_log2(debug, "正在分发块 " SEGMENT_FORMAT, SEGMENT_FORMAT_ARGS(segment));
        //统计信息
        InternalStats::inc_num_segments_from_manager();
        return segment;
    }

    void ContextHolder::split_segment(Segment *segment,
                                      SegmentLevel target_level) {
        assert_lock_strong(Metaspace_lock);
        assert(segment->is_free(), "仅仅可以切割空闲segment");
        assert(segment->level() < target_level, "segment的等级应小于目标等级");
        assert(segment->prev() == nullptr &&
               segment->next() == nullptr,
               "内存块应不隶属与链表,才可以切割");
        assert(level_is_valid(target_level), "目标segment等级错误");
        meta_log2(debug, "正在将" SEGMENT_FORMAT "切割到" SEGMENT_LV_FORMAT,
                  SEGMENT_FORMAT_ARGS(segment), target_level);
        auto region = segment->container()->region_by_pointer(segment->base());
        region->split(target_level, segment, this->_segment_mgr);
        assert(segment->level() == target_level, "切割不可能失败");
        //增加统计信息
        InternalStats::inc_num_segments_splits();
    }

    Segment *ContextHolder::attempt_merge_segment(Segment *segment) {
        assert(segment->is_free() && segment->used_bytes() == 0, "仅可以合并空闲的segment");
        //记录原本内存块原始的等级
        const auto origin_level = segment->level();
        Segment *merged_segment = nullptr;
        /**
         * 不是根块才可以进行合并
         */
        if (!segment->is_root_segment()) {
            auto root = segment->container()->region_by_pointer(segment->base());
            merged_segment = root->merge(segment, this->_segment_mgr);
        }
        /**
         * 如果有合并的情况
         */
        if (merged_segment) {
            InternalStats::inc_num_segments_merges();
            //合并后的块等级应该小于原本块等级
            assert(merged_segment->level() < origin_level, "健全");
            meta_log2(debug, "合并成为segment:" SEGMENT_FORMAT, SEGMENT_FORMAT_ARGS(merged_segment));
            return merged_segment;
        }
        return segment;
    }

    /**
     * 用于打印的辅助工具
     * @param out
     * @param bytes1
     * @param bytes2
     */
    static void print_bytes_delta(CharOStream *out, size_t bytes1, size_t bytes2) {
        if (bytes1 == bytes2) {
            out->print_human_bytes(bytes1);
            out->print_raw("( 无改变 )");
        } else {
            out->print_human_bytes( bytes1);
            out->print_raw("->");
            out->print_human_bytes( bytes2);
            out->print_raw("(");
            if (bytes2 <= bytes1) {
                out->print_raw("-");
                out->print_human_bytes( bytes1 - bytes2);
            } else {
                out->print_raw("+");
                out->print_human_bytes( bytes2 - bytes1);
            }
            out->print_raw(")");
        }
    }

    void ContextHolder::purge() {
        MutexLocker fcl(Metaspace_lock);
        meta_log(info, "回收内存中...");
        const auto reserved_before = this->_volume_list->reserved_bytes();
        const auto committed_before = this->_volume_list->committed_bytes();
        /**
         * 迭代 现在管理的空闲块
         * 看看是否有内存块已提交的大小 >= 内存的提交粒度
         * 那么将执行取消映射 释放内存
         */
        const auto max_level = bytes_to_level(CommitGranuleBytes);
        for (SegmentLevel i = SegmentLevel::LV_LOWEST; i <= max_level; i = (SegmentLevel)((SegementLevel_t)i + 1)) {
            /**
             * 因为我们在这个级别取消了所有的数据块，
             * 所以我们没有打破“已提交的数据块位于列表的前面”的条件。
             */
            for (auto segment = this->_segment_mgr->first_at_level(i);
                 segment != nullptr;
                 segment = segment->next()) {
                segment->uncommit();
            }
        }


        const auto reserved_after = this->_volume_list->reserved_bytes();
        const auto committed_after = this->_volume_list->committed_bytes();
        /**
         * 日志的打印
         */
        if (reserved_after == reserved_before && committed_after == committed_before) {
            meta_log(info, "没有内存被回收");
            return;
        }
        meta_log_stream(info);
        if (log.is_enable()) {
            log.print_cr(LOG_FMT ": 回收内存已结束", LOG_FMT_ARGS);
            /**
             * reserved ??->??(+15KB)
             */
            log.print_raw("reserved: ");
            print_bytes_delta(&log, reserved_before, reserved_after);
            log.cr();
            log.print_raw("committed: ");
            print_bytes_delta(&log, committed_before, committed_after);
            log.cr();
        }
    }

    void ContextHolder::print_on(CharOStream *out) const {
        this->_volume_list->print_on(out);
        this->_segment_mgr->print_on(out);
    }

    void ContextHolder::return_segment_with_lock(Segment *segment) {
        //输出日志信息
        meta_log2(debug, "正在归还 " SEGMENT_FORMAT, SEGMENT_FORMAT_ARGS(segment));
        assert(segment->is_free() || segment->is_inuse(), "Segment状态错误");
        assert(segment->next() == nullptr, "Segment还在链表中");
        //设置内存块的状态
        segment->set_free();
        segment->reset_used_top();
        //尝试合并空闲块
        segment = this->attempt_merge_segment(segment);
        this->_segment_mgr->add(segment);
        meta_log2(debug, "已归还segment:" SEGMENT_FORMAT,
                  SEGMENT_FORMAT_ARGS(segment));
        /**
         * 增加统计信息
         */
        InternalStats::inc_num_segments_to_manager();
    }

    void ContextHolder::init_context() {
        if (ContextHolder::_context == nullptr) {
            auto manager = new SegmentManager();
            auto list = new VolumeList();
            ContextHolder::_context = new ContextHolder(manager,list);
        }
    }
#ifdef DIAGNOSE
    void ContextHolder::verify() {
        auto context = ContextHolder::context();
        assert(context!= nullptr,"not initialize");
        context->_volume_list->verify();
    }
#endif

}

