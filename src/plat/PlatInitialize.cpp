//
// Created by aurora on 2024/6/29.
//
#include "plat/PlatInitialize.hpp"
#include "inner_os.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "plat/stream/FileCharOStream.hpp"
#include "plat/logger/LogTagSet.hpp"
#include "plat/thread/OSThread.hpp"

void PlatInitialize::initialize(ticks_t vm_start_time,
                                const char **tags_name,
                                uint16_t max_tags,
                                OSThread *os_thread) {
    os::time_initialize(vm_start_time);
    os::native_prio_initialize();
    MemoryTracer::initialize();
    ArenaChunkPool::initialize();
    assert(max_tags < PREFIX_LOG_TAG(no_tag), "too many tags");
    LogTagSet::_tags_name = tags_name;
    LogTagSet::_tags_max = max_tags;
    OSThread::attach_main_thread(os_thread);
}

void PlatInitialize::destroy() {
    MemoryTracer::flush();
    FileCharOStream::flush_default_stream();
}
