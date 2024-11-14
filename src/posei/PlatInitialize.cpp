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
                                OSThread *os_thread) {
    os::time_initialize(vm_start_time);
    os::native_prio_initialize();
    MemoryTracer::initialize();
    ArenaChunkPool::initialize();
    OSThread::attach_main_thread(os_thread);
}

void PlatInitialize::destroy() {
    MemoryTracer::flush();
    FileCharOStream::flush_default_stream();
}
