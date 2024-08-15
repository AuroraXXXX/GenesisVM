//
// Created by aurora on 2024/2/13.
//

#include <cstring>
#include "kernel/memory/MetaspaceArena.hpp"
#include "kernel/metaspace/Arena.hpp"
#include "kernel/utils/locker.hpp"
#include "plat/logger/log.hpp"
#include "kernel/metaspace/constants.hpp"

static metaspace::SegmentLevel g_sequ_boot[] = {
        metaspace::SegmentLevel::LV_4M,
        metaspace::SegmentLevel::LV_1M
};

static metaspace::SegmentLevel g_sequ_standard[] = {
        metaspace::SegmentLevel::LV_4K,
        metaspace::SegmentLevel::LV_4K,
        metaspace::SegmentLevel::LV_4K,
        metaspace::SegmentLevel::LV_8K,
        metaspace::SegmentLevel::LV_16K
};


#define DEFINE_ARENA_GROWTH_POLICY(what) \
        static metaspace::ArenaGrowthPolicy         \
        alloc_sequence_##what(g_sequ_##what, sizeof(g_sequ_##what)/sizeof(metaspace::SegmentLevel)); \
        return &alloc_sequence_##what;

extern metaspace::ArenaGrowthPolicy *arena_policy_for_standard() {
    DEFINE_ARENA_GROWTH_POLICY(standard)
}

extern metaspace::ArenaGrowthPolicy *arena_policy_for_boot() {
    DEFINE_ARENA_GROWTH_POLICY(boot)
}

MetaspaceArena::MetaspaceArena(MetaspaceType space_type, Mutex *lock) :
        _mutex(lock),
        _arena(nullptr) {
    metaspace::ArenaGrowthPolicy *policy = nullptr;
    switch (space_type) {

        case MetaspaceType::Boot:
            policy = arena_policy_for_boot();
            break;
        case MetaspaceType::Standard:
            policy = arena_policy_for_standard();
            break;
        default:
            should_not_reach_here();
    }
    this->_arena = new metaspace::Arena(policy);
}


MetaspaceArena::~MetaspaceArena() {
    MutexLocker locker(this->_mutex);
    delete this->_arena;
}


void *MetaspaceArena::allocate(size_t bytes) {
    void *ptr = nullptr;
    {
        MutexLocker locker(this->_mutex);
        ptr = this->_arena->allocate(bytes);
    }
    if (ptr != nullptr) {
        ::memset(ptr, 0, bytes);
        log_trace(metaspace)("MetaspaceArena::allocate:  [" PTR_FORMAT "," PTR_FORMAT ").",
                             (uintptr_t) ptr,
                             (uintptr_t) ptr + bytes);
    }
    return ptr;
}

void MetaspaceArena::deallocate(void *ptr, size_t bytes) {
    MutexLocker locker(this->_mutex);
    this->_arena->deallocate(reinterpret_cast<void *>(ptr), bytes);
}

void MetaspaceArena::usage_numbers(
        size_t *used_bytes,
        size_t *committed_bytes,
        size_t *capacity_bytes) {
    MutexLocker locker(this->_mutex);
    this->_arena->usage_numbers(used_bytes, committed_bytes, capacity_bytes);
}



