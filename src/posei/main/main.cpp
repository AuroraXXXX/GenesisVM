//
// Created by aurora on 2024/6/29.
//
#include "posei/init/main.hpp"
#include "posei/os.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "posei/stream/FileCharOStream.hpp"
#include "posei/thread/OSThread.hpp"

void posei_init(ticks_t vm_start_time,
                OSThread *os_thread) {
    os::time_initialize(vm_start_time);
    os::native_prio_initialize();
    MemoryTracer::initialize();
    ArenaChunkPool::initialize();
    OSThread::attach_main_thread(os_thread);
}

void posei_destroy() {
    MemoryTracer::flush();
    FileCharOStream::flush_default_stream();
}
