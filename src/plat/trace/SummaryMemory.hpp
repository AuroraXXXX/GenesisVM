//
// Created by aurora on 2024/6/25.
//

#ifndef PLAT_SUMMARY_MEMORY_HPP
#define PLAT_SUMMARY_MEMORY_HPP

#include "stdtype.hpp"
#include "MemoryTracer.hpp"
#include "plat/mem/allocation.hpp"
class OStream;
class SummaryMemory {
private:
    struct Unit {
        volatile size_t _virtual_reserved;
        volatile size_t _virtual_committed;
        volatile size_t _native_alloc;
        volatile size_t _native_count;
        volatile size_t _arena_alloc;
        volatile size_t _arena_count;
        explicit Unit()noexcept;
    };
    constexpr static inline auto max_tag = (int32_t)(MEMFLAG::num_of_type);
    static Unit _unit[max_tag];
public:


    static void summary(MEMFLAG F,
                        MemoryTracer::OperationType type,
                        size_t bytes);

    static void output(OStream* stream);
};

#endif //PLAT_SUMMARY_MEMORY_HPP
