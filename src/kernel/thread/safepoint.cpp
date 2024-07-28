//
// Created by aurora on 2022/12/19.
//

#include "safepoint.hpp"
#include "kernel/utils/log.hpp"
#include "kernel_mutex.hpp"
#include "plat/os/time.hpp"
#include "plat/thread/WaitBarrier.hpp"
#include "kernel/thread/PlatThread.hpp"
#include "plat/thread/SpinYield.hpp"
#include "kernel/thread/LangThread.hpp"

#include "StillRunningThreadClosure.hpp"
volatile SafepointSynchronize::SynchronizeState SafepointSynchronize::_state =
        SynchronizeState::not_synchronized;
TimeStamp SafepointSynchronize::_safe_point_beg_time;
TimeStamp SafepointSynchronize::_safe_point_end_time;
WaitBarrier SafepointSynchronize::_wait_barrier;
volatile int32_t SafepointSynchronize::_safe_point_check = 0;


void SafepointSynchronize::begin() {
    assert(state() == SynchronizeState::not_synchronized, "应未设置同步才可以开始");
    assert(PlatThread::current()->is_VM_thread(), "仅仅VMThread可以调用");
    //开始记录时间
    _safe_point_beg_time.update_realtime_ticks();
    /**
     * 调用 LangThreadList_lock
     * 我们确保从此刻到退出安全点期间没有LangThread被创建和销毁
     */
    LangThreadList_lock->lock();
    // auto lang_threads = LangThread::threads_num();
    log_debug(safepoint)("正在使用%s wait barrier 初始化安全点(Safepoint)同步器.",
                         WaitBarrier::description());

    /**
     * 表明现在正在正在同步
     */
    _wait_barrier.arm(_safe_point_check + 1);
    //此处应必须是偶数 说明我们之前没有进行安全点操作
    assert((_safe_point_check & 0x01) == 0, "健全");
    OrderAccess::fetch_and_add(&_safe_point_check, 1);
    OrderAccess::compile_barrier();
    _state = SynchronizeState::synchronizing;
    /**
     * 强制使用屏障 不允许进行指令乱排序
     */
    OrderAccess::fence();
    size_t init_running = 0;
    auto iteration = SafepointSynchronize::synchronize_threads(
                                                               &init_running);
    log_info(safepoint)("safepoint同步线程:起始运行线程:%ld,迭代次数:%ld.",
                        init_running,
                        iteration);
    assert(LangThreadList_lock->owned_by_self(), "我们必须持有这个锁");
    OrderAccess::store(&_state, SynchronizeState::synchronized);
}

void SafepointSynchronize::end() {
    assert(LangThreadList_lock->owned_by_self(), "我们必须持有这个锁");
    //  assert(Thread::current()->is_VM_thread(), "仅仅VMTread可以执行安全点");
    OrderAccess::fence();
    assert(_state == SynchronizeState::synchronized, "必须之前是synchronized才可以结束安全点");
    OrderAccess::store(&_state, SynchronizeState::not_synchronized);
    //此处必须是奇数
    assert((_safe_point_check & 0x01) == 0, "健全");
    OrderAccess::fetch_and_add(&_safe_point_check, 1);
    OrderAccess::fence();
    /**
     * 释放Lang线程创建和销毁的锁 允许语言层面的线程创建和销毁
     */
    LangThreadList_lock->lock();

    //唤醒所有等待在_wait_barrier上的锁
    _wait_barrier.disarm();
    _safe_point_end_time.update_realtime_ticks();
    log_info(safepoint)("safepoint持续时间:%ld ns.",
                        _safe_point_end_time.during_ns(_safe_point_beg_time));
}



int SafepointSynchronize::synchronize_threads(
        size_t *init_running) {
    StillRunningThreadClosure closure;
    PlatThread::thread_do_user(&closure);
    auto still_running = closure.still_num();
    PlatThread* still_running_list= closure.still_list();

    *init_running = still_running;
    //迭代的次数
    int iterations = 1;
    /**
     * 所有的语言层面的线程都停止了运行
     */
    if (still_running <= 0) {
        return iterations;
    }
    /**
     * 下面的就是还有运行的线程
     * 我们需要自旋 进行等待
     */
    assert(still_running_list != nullptr, "异常");
    const auto start_time = os::current_stamp();
    do {
        closure.clear();
        PlatThread::thread_do(&still_running_list,&closure);
        still_running = closure.still_num();
        still_running_list = closure.still_list();

        if (still_running > 0) {
            //检查完毕 还是有正在运行的线程 那么我们睡眠一会进行等待
            SafepointSynchronize::back_off(start_time);
        }
        //增加一次迭代的次数
        ++iterations;

    } while (still_running > 0);


    //返回迭代次数
    return iterations;
}


void SafepointSynchronize::back_off(ticks_t start_time) {
    /**
     * 当距离开始时间小于1 ms时候 每次睡眠 10 微秒
     * 超过1毫秒时候 每次睡眠1毫秒
     */
    constexpr auto ns_per_ms = TicksPerMS / TicksPerNS;
    constexpr auto ns_per_us = TicksPerUS / TicksPerNS;
    if (os::current_stamp() - start_time < ns_per_ms) {
        SpinYield::sleep(10 * ns_per_us);
    } else {
        SpinYield::sleep(ns_per_ms);
    }
}
