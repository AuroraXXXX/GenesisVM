//
// Created by aurora on 2024/6/29.
//
#include "platform/main/init.hpp"
#include "platform/os.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "platform/stream/FileCharOStream.hpp"
#include "platform/concurrent/OSThread.hpp"
#include "platform/concurrent/Monitor.hpp"
#include "platform/log.hpp"
#include "VMThread.hpp"
#include "PeriodicThread.hpp"
void Platform::pre_initialize()  {
    auto stamp =  os::current_stamp();
    os::time_initialize(stamp);
    os::native_prio_initialize();
    MemoryTracer::initialize();
    //1. 先进性初始化内存池
    ArenaChunkPool::initialize();

}

void Platform::destroy() {
    //1. 判断当前线程是否是daemon线程 ,如果是那么就需要等待所有非守护线程执行完毕， 否则可以剩下我们
   const   auto survive_num = OSThread::current()->is_daemon_thread()? 0 : 1;
    {
        MonitorLocker lock(UserThread::locker());
        //2. 等待所有非守护线程执行完毕
        while (UserThread::non_daemon_of_user_thread_count() > survive_num) {
            lock.wait();
        }
    }
    //3 执行后续销毁操作

    MemoryTracer::flush();
    FileCharOStream::flush_default_stream();
    OSThread::main_thread()->post_run();
    log_info(platform)("main thread is exited.");
}
void Platform::global_initialize() {
    //2. 创建表示MAIN 的线程
    auto os_thread = new MainThread();
    OSThread::attach_main_thread(os_thread);
    //3. 启动定时任务
    PeriodicThread::create();
    //4. 启动定时任务
    PeriodicThread::start();
    //5 启动VMThread
    VMThread::create();
}
void Platform::before_destroy() {
    //1. 停止VMThread
    VMThread::destroy();
    //2. 停止定时任务
    PeriodicThread::stop();
}


