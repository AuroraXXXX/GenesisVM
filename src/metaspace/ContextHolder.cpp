//
// Created by aurora on 2025/3/25.
//

#include "ContextHolder.hpp"

metaspace::VolumeList* metaspace::ContextHolder::_volume_list = nullptr;
metaspace::LevelSegmentArray* metaspace::ContextHolder::_level_segment_array = nullptr;

void metaspace::ContextHolder::return_segment(metaspace::Segment *segment) {

}
