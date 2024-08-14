//
// Created by aurora on 2024/2/1.
//
#include "kernel/metaspace/constants.hpp"
#include "plat/stream/CharOStream.hpp"
#include "BlockManager.hpp"
#include "plat/utils/align.hpp"
namespace metaspace {
    extern SegmentLevel bytes_to_level(size_t bytes) {
        assert(bytes <= metaspace::RegionBytes,
               "内存块" SIZE_FORMAT "过大，超过允许范围.");
        if (bytes <= level_to_bytes(SegmentLevel::LV_HIGHEST)) {
            return SegmentLevel::LV_HIGHEST;
        }
        size_t aligned_bytes = round_up_power_of_2(bytes);
        auto level =  (SegementLevel_t)SegmentLevel::LV_NUM - 1 - log2i_exact<size_t>(aligned_bytes);
        return (SegmentLevel) level;
    }

    extern size_t get_raw_byte_for_requested(size_t requested_bytes) {
        //首先至少应该大于BlockManager::MIN_BYTES要求的最小值
        auto bytes = MAX2(requested_bytes, BlockManager::MIN_BYTES);
        //进行对齐的
        bytes = align_up(bytes, MetaAlignedBytes);
        return bytes;
    }

    extern void print_on_using_constants_setting(CharOStream *st) {
        const char *unit;
        st->print_raw(" - commit_granule_bytes: ");
        st->print_human_bytes(CommitGranuleBytes);
        st->cr();

        st->print_raw(" - meta_align_bytes: ");
        st->print_human_bytes(MetaAlignedBytes);
        st->cr();

        st->print_raw(" - volume_default_bytes: ");
        st->print_human_bytes(VolumeDefaultBytes);
        st->cr();

        st->print_cr(" - enlarge_segment_in_place: %d.", (int) EnlargeSegmentInPlace);
    }
}