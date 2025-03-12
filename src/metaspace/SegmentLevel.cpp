#include "metaspace/SegmentLevel.hpp"
#include "platform/utils/align.hpp"
#include "metaspace/Metaspace.hpp"
SegmentLevel::SegmentLevel_t SegmentLevel::get_optimal_level(size_t bytes){
    assert(bytes <= metaspace::RegionBytes,
        "内存块" SIZE_FORMAT "过大，超过允许范围.");
    if (bytes <= get_bytes(SegmentLevel::LV_HIGHEST)) {
        return SegmentLevel::LV_HIGHEST;
    }
    size_t aligned_bytes = round_up_power_of_2(bytes);
    auto level =  (SegementLevel_t)SegmentLevel::LV_NUM - 1 - log2i_exact<size_t>(aligned_bytes);
    return (SegmentLevel) level;
}

size_t SegmentLevel::get_bytes(SegmentLevel_t level){
    assert(level >= SegmentLevel::LV_LOWEST && level <= SegmentLevel::LV_HIGHEST,"check");
    return metaspace::RegionBytes >> level;
}