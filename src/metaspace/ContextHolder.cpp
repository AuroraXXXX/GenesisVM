//
// Created by aurora on 2025/3/25.
//

#include "ContextHolder.hpp"
#include "meta_log.hpp"
#include "Segment.hpp"
#include "platform/concurrent/Mutex.hpp"
#include "VolumeList.hpp"
#include "LevelSegmentArray.hpp"
#define LOG_FMT         "ContextHolder @" PTR_FORMAT
#define LOG_FMT_ARGS    this
Mutex metaspace::ContextHolder::Metaspace_lock("Metaspace_lock");

void metaspace::ContextHolder::return_segment(metaspace::Segment *segment) {
//    assert(segment->_state == Segment::State::Free || segment->_state == Segment::State::InUse , "segment status is error");
//    assert(segment->link_list_node()->is_clear(),"must be");
//    // 进行日志记录
//    meta_log_stream(trace);
//    if (log.is_enable()) {
//        log.print("return segment ");
//        segment->print_on(&log);
//    }
//    //设置内存块状态
//    segment->_state = Segment::State::Free;
//    //重置使用的内存
//    segment->_used_bytes = 0;


}

void metaspace::ContextHolder::global_initialize() {
    _volume_list = new VolumeList();
    _level_segment_array = new LevelSegmentArray();

}

namespace metaspace {
    /**
     * 在管理的空闲Segment中查找满足条件的Segment
     * @param level_segment_array 管理segment的工具
     * @param preferred_level 希望的内存块等级
     * @param max_level (最少内存块大小)最大内存块等级
     * @param suggest_min_committed_bytes 最少应该被提交的内存大小 单位字节
     * @return 满足条件的Segment，没有返回nullptr
     */
    Segment *search_satisfy_segment_in_free(
            LevelSegmentArray *level_segment_array,
            SegmentLevel_t preferred_level,
            SegmentLevel_t max_level,
            size_t suggest_min_committed_bytes) {
        /**
         * 逻辑判断
         */
        assert(SegmentLevel::get_bytes(max_level) >= suggest_min_committed_bytes, "must be");
        assert(SegmentLevel::get_bytes(preferred_level) >= SegmentLevel::get_bytes(max_level), "must be");
        Segment *segment;
        /**
         * 1.首先寻找已提交内存满足min_committed_bytes(请求时需求的最小已提交内存)的内存块
         * 而
         * 内存块的保留地址空间从大到小，即从preferred_level寻找到max_level。
         * 避免吞噬小的内存块。导致内存一些非常小内存块
         */
        segment = level_segment_array->search_segment_ascending(
                preferred_level,
                max_level,
                suggest_min_committed_bytes);
        if (segment != nullptr) {
            return segment;
        }
        /**
         * 2.在较大的内存块中搜寻已提交内存满足min_committed_bytes(请求时需求的最小已提交内存)的内存块
         */
        segment = level_segment_array->search_segment_descending(
                preferred_level,
                suggest_min_committed_bytes);
        if (segment) {
            return segment;
        }
        /**
         * 3 再次重复进行第一次的查找 但是这次只要求最小提交内存满足要求即可
         *  不再可虑是否可能吞噬小的内存块
         */
        segment = level_segment_array->search_segment_ascending(
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
        segment = level_segment_array->search_segment_ascending(preferred_level,
                                                                max_level,
                                                                0);
        if (segment) {
            return segment;
        }

        /**
         * 5 满足要求的虚拟地址空间也没有找到 那么就搜寻更大的虚拟地址空间
         */
        segment = level_segment_array->search_segment_descending(preferred_level,
                                                                 0);
        return segment;
    }

    Segment * ContextHolder::get_segment(SegmentLevel_t preferred_level,
                               SegmentLevel_t max_level,
                               size_t min_committed_bytes) {
        /**
         * 首先希望的内存块等级 应该大于等于 最大的内存块等级
         * 即希望的内存块大小应大于等于至少内存块大小
         *
         * 第二的断言 要求 最小提交大小 和最大内存块等级的正确性
         */
        assert(preferred_level <= max_level, "健全");
        assert(SegmentLevel::get_bytes(max_level) >= min_committed_bytes, "must be");
        assert(SegmentLevel::get_bytes(preferred_level) >= SegmentLevel::get_bytes(max_level), "must be");
        // 获取全局锁
        MutexLocker locker(ContextHolder::global_locker());
        /**
         * 日志的输出
         */
        meta_log2(info, "request segment pref_level:" SEGMENT_LV_FORMAT
                ",max_level:" SEGMENT_LV_FORMAT
                ",min_committed_bytes:" SIZE_FORMAT,
                  preferred_level, max_level, min_committed_bytes);
         auto segment = search_satisfy_segment_in_free(
                this->_level_segment_array,
                preferred_level,
                max_level,
                min_committed_bytes);
        if (segment){
            meta_log(info,"already get free segment from level_segment_array");
        }else{
            /**
            * 到这里 说明整个空闲内存块管理器
            * 无法获取满足要求的内存块
            * 我们需要申请得到一个根块 然后进行切分
            *
            */
            segment =  this->_volume_list->allocate_root_segment();
        }
        return nullptr;
    }
}
