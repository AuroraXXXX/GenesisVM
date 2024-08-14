//
// Created by aurora on 2022/12/24.
//

#include "kernel/metaspace/InternalStats.hpp"
#include "plat/stream/CharOStream.hpp"
namespace metaspace {
    /**
     * 对静态变量进行定义
     */
#define DEFINE_COUNTER(name, human)  std::atomic<uint64_t> InternalStats::_##name;
#define DEFINE_ATOMIC_COUNTER(name, human)  uint64_t InternalStats::_##name;
    ALL_INTERNAL_STATS(DEFINE_COUNTER, DEFINE_ATOMIC_COUNTER)
#undef DEFINE_ATOMIC_COUNTER
#undef DEFINE_COUNTER

    void InternalStats::print_on(CharOStream *out) {
#define PRINT_COUNTER(name, human) out->print_cr("%s:" UINTX_FORMAT "(%s).",\
                                    #name,                                   \
                                    InternalStats::name(),                   \
                                    human);
        ALL_INTERNAL_STATS(PRINT_COUNTER, PRINT_COUNTER)
#undef PRINT_COUNTER
    }
}