//
// Created by aurora on 2024/6/25.
//

#ifndef PLAT_SUMMARY_MEMORY_HPP
#define PLAT_SUMMARY_MEMORY_HPP

#include "platform/typedef.hpp"
#include "MemoryTracer.hpp"
#include "platform/allocation.hpp"
#include <atomic>
class OStream;
class SummaryMemory {
private:
    struct Unit {
        std::atomic<size_t> _virtual_reserved;
        std::atomic<size_t>_virtual_committed;
        std::atomic<size_t>_native_alloc;
        std::atomic<size_t> _native_count;
        std::atomic<size_t> _arena_alloc;
        std::atomic<size_t> _arena_count;
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
