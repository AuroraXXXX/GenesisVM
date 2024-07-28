//
// Created by aurora on 2024/6/25.
//
#include <cstring>
#include "MemoryTracer.hpp"
#include "SummaryMemory.hpp"
#include "DetailLogMemory.hpp"
#include "plat/globals.hpp"
#include "plat/utils/robust.hpp"

MemoryTracer::NMT_Level MemoryTracer::_nmt_level = NMT_Level::unknown;

void MemoryTracer::record(MEMFLAG F,
                          MemoryTracer::OperationType type,
                          void *addr,
                          size_t bytes,
                          const NativeCallStack &call_stack) {
    switch (MemoryTracer::_nmt_level) {
        case NMT_Level::unknown:
        case NMT_Level::off:
            return;
        case NMT_Level::detail:
            DetailLogMemory::detail_log(F, type, addr, bytes, call_stack);
        case NMT_Level::summary:
            SummaryMemory::summary(F, type, bytes);
            return;
    }
}

void MemoryTracer::initialize() {
    auto level = global::NMTLevel;
    if (::strcmp(level, "off") == 0) {
        MemoryTracer::_nmt_level = NMT_Level::off;
    } else if (::strcmp(level, "summary") == 0) {
        MemoryTracer::_nmt_level = NMT_Level::summary;
    } else if (::strcmp(level, "detail") == 0) {
        MemoryTracer::_nmt_level = NMT_Level::detail;
        DetailLogMemory::global_initialize();
    } else {
        // 必须在三者选择1个
        guarantee(false, "Native Memory Trace Level is error, must be selected from off, summary and detail.");
    }
}

void MemoryTracer::flush() {
    const auto stream = DetailLogMemory::stream();
    DetailLogMemory::flush();
    if (stream != nullptr) {
       SummaryMemory::output(stream);
    }
}
