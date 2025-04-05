//
// Created by aurora on 2025/3/25.
//

#include "ContextHolder.hpp"
#include "meta_log.hpp"
#include "Segment.hpp"
#include "platform/concurrent/Mutex.hpp"

metaspace::VolumeList *metaspace::ContextHolder::_volume_list = nullptr;
metaspace::LevelSegmentArray *metaspace::ContextHolder::_level_segment_array = nullptr;
Mutex metaspace::ContextHolder::Metaspace_lock("Metaspace_lock");

void metaspace::ContextHolder::return_segment(metaspace::Segment *segment) {
    assert(segment->_state == Segment::State::Free || segment->_state == Segment::State::InUse , "segment status is error");
    assert(segment->link_list_node()->is_clear(),"must be");
    // 进行日志记录
    meta_log_stream(trace);
    if (log.is_enable()) {
        log.print("return segment ");
        segment->print_on(&log);
    }
    //设置内存块状态
    segment->_state = Segment::State::Free;
    //重置使用的内存
    segment->_used_bytes = 0;


}
