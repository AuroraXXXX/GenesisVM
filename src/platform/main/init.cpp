//
// Created by aurora on 2024/6/29.
//
#include "platform/main/init.hpp"
#include "platform/os.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "platform/stream/FileCharOStream.hpp"
#include "platform/concurrent/OSThread.hpp"

void platform_init() {
    auto stamp =  os::current_stamp();
    os::time_initialize(stamp);
    os::native_prio_initialize();
    MemoryTracer::initialize();
    //1. 先进性初始化内存池
    ArenaChunkPool::initialize();
    //2. 创建表示MAIN 的线程
    auto os_thread = new MainThread();
    OSThread::attach_main_thread(os_thread);
}

void platform_destroy() {
    MemoryTracer::flush();
    FileCharOStream::flush_default_stream();
    OSThread::main_thread()->post_run();
}
