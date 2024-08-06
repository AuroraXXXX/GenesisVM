//
// Created by aurora on 2024/2/1.
//
#include "constants.hpp"
#include "plat/stream/CharOStream.hpp"
#include "BlockManager.hpp"

namespace metaspace {


    extern size_t get_raw_byte_for_requested(size_t requested_bytes) {
        //首先至少应该大于BlockManager::MIN_BYTES要求的最小值
        auto bytes = MAX2(requested_bytes, BlockManager::MIN_BYTES);
        //进行对齐的
        bytes = align_up(bytes, MetaAlignedBytes);
        return bytes;
    }

    extern void print_on_using_constants_setting(CharOStream *st) {
        const char *unit;
        st->print_raw(" - commit_granule_bytes: " );
        st->print_human_bytes(CommitGranuleBytes);
        st->cr();

        st->print_raw(" - meta_align_bytes: " );
        st->print_human_bytes(MetaAlignedBytes);
        st->cr();

        st->print_raw(" - volume_default_bytes: " );
        st->print_human_bytes(VolumeDefaultBytes);
        st->cr();

        st->print_cr(" - enlarge_segment_in_place: %d.", (int) EnlargeSegmentInPlace);
    }
}