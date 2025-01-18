//
// Created by aurora on 2024/6/29.
//
#include "platform/main/init.hpp"
#include "platform/os.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "platform/stream/FileCharOStream.hpp"
#include "platform/thread/OSThread.hpp"

void platform_init(ticks_t vm_start_time,
                OSThread *os_thread) {
    os::time_initialize(vm_start_time);
    os::native_prio_initialize();
    MemoryTracer::initialize();
    ArenaChunkPool::initialize();
    OSThread::attach_main_thread(os_thread);
}

void platform_destroy() {
    MemoryTracer::flush();
    FileCharOStream::flush_default_stream();
}
