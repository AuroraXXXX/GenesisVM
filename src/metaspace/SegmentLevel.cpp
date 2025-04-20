

#include "SegmentLevel.hpp"
#include "platform/utils/align.hpp"
#include "Setting.hpp"
metaspace::SegmentLevel_t metaspace::SegmentLevel::get_optimal_level(size_t bytes){
    assert(bytes <= metaspace::Setting::RegionBytes,
        "内存块" SIZE_FORMAT "过大，超过允许范围.");
    if (bytes <= get_bytes(SegmentLevel::LV_HIGHEST)) {
        return SegmentLevel::LV_HIGHEST;
    }
    size_t aligned_bytes = round_up_power_of_2(bytes);
    auto level =  (SegmentLevel_t)SegmentLevel::LV_NUM - 1 - log2i_exact<size_t>(aligned_bytes);
    return (SegmentLevel_t) level;
}

size_t metaspace::SegmentLevel::get_bytes(metaspace::SegmentLevel_t level){
    assert(SegmentLevel::is_valid(level),"check");
    return metaspace::Setting::RegionBytes >> level;
}