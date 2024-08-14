//
// Created by aurora on 2022/12/23.
//
#ifndef KERNEL_METASPACE_ARENA_GROWTH_POLICY_HPP
#define KERNEL_METASPACE_ARENA_GROWTH_POLICY_HPP

#include "kernel/metaspace/constants.hpp"

namespace metaspace {

    /**
     * 内存块Segment的增长策略的
     */
    class ArenaGrowthPolicy {
    private:
        SegmentLevel *const _entries;
        const size_t _num_entries;
    public:
        inline explicit ArenaGrowthPolicy(
                SegmentLevel *entries,
                size_t num_entries):
                _entries(entries),
                _num_entries(num_entries){
        };

        SegmentLevel get_level_by_step(size_t num_allocated){
            if (num_allocated >= this->_num_entries) {
                num_allocated = this->_num_entries - 1;
            }
            return this->_entries[num_allocated];
        };
    };
}
#endif //KERNEL_METASPACE_ARENA_GROWTH_POLICY_HPP
