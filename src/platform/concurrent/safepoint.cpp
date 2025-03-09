//
// Created by aurora on 2022/12/19.
//

#include "platform/concurrent/safepoint.hpp"
#include "platform/log.hpp"
#include "platform/os.hpp"
#include "platform/concurrent/Mutex.hpp"
#include "platform/utils/robust.hpp"
#include "platform/concurrent/OSThread.hpp"
#include "platform/concurrent/SpinYield.hpp"
std::atomic<Safepoint::SynchronizeState> Safepoint::_state = SynchronizeState::not_synchronized;
ticks_t Safepoint::_beg_time = 0;
ticks_t Safepoint::_end_time = 0;
WaitBarrier Safepoint::_wait_barrier;
std::atomic<int32_t> Safepoint::_safe_point_check = 0;


void Safepoint::begin() {
    assert(Safepoint::_state.load() == SynchronizeState::not_synchronized, "应未设置同步才可以开始");
    //开始记录时间
    Safepoint::_beg_time = os::current_stamp();
    /**
     * 调用 LangThreadList_lock
     * 我们确保从此刻到退出安全点期间没有LangThread被创建和销毁
     */
    UserThread::locker()->lock();
    log_debug(safepoint)("正在使用%s wait barrier 初始化安全点(Safepoint)同步器.",
                      WaitBarrier::description());

    /**
     * 表明现在正在正在同步
     */
    Safepoint::_wait_barrier.arm(_safe_point_check + 1);
    //此处应必须是偶数 说明我们之前没有进行安全点操作
    assert((Safepoint::_safe_point_check.load() & 0x01) == 0, "健全");
    Safepoint::_safe_point_check.fetch_add(1);
    Safepoint::_state.store(SynchronizeState::synchronizing);
    /**
     * 强制使用屏障 不允许进行指令乱排序
     */
    std::atomic_thread_fence(std::memory_order::seq_cst);
    size_t init_running = 0;
    auto iteration = Safepoint::synchronize_threads(&init_running);
    log_info(safepoint)("safepoint同步线程:起始运行线程:%ld,迭代次数:%ld.",
                     init_running,
                     iteration);
    assert(UserThread::locker()->owned_by_self(), "我们必须持有这个锁");
    Safepoint::_state.store(SynchronizeState::synchronized);

}

void Safepoint::end() {
    assert(UserThread::locker()->owned_by_self(), "我们必须持有这个锁");

    assert(Safepoint::_state.load() == SynchronizeState::synchronized, "必须之前是synchronized才可以结束安全点");
    Safepoint::_state.store(SynchronizeState::not_synchronized);
    //此处必须是奇数
    assert((Safepoint::_safe_point_check.load() & 0x01) == 1, "健全");
    Safepoint::_safe_point_check.fetch_add(1);
    /**
     * 释放Lang线程创建和销毁的锁 允许语言层面的线程创建和销毁
     */
    UserThread::locker()->unlock();

    //唤醒所有等待在_wait_barrier上的锁
    Safepoint::_wait_barrier.disarm();
    Safepoint::_end_time = os::current_stamp();
    log_info(safepoint)("safepoint持续时间:%ld ns.",(Safepoint::_end_time - Safepoint::_beg_time));
}

int Safepoint::synchronize_threads(
        size_t *init_running) {
    UserThread* still_list = nullptr;
    size_t still_list_num = 0;
    auto iter_still_running_thread_func = [&still_list,&still_list_num](UserThread* current,size_t index){
        if(current->is_running_state()){
            //将其存放到仍然存货的链表中
            current->set_stilling_next(still_list);
            still_list = current;
            still_list_num += 1;
        }
        //表示遍历全部
        return true;
    };

    //先遍历一遍
    UserThread::list().iter(iter_still_running_thread_func);

    *init_running = still_list_num;
    //迭代的次数
    int iterations = 1;
    /**
     * 所有的语言层面的线程都停止了运行 那么就进行返回
     */
    if (still_list_num <= 0) {
        return iterations;
    }
    /**
     * 下面的就是还有运行的线程
     * 我们需要自旋 进行等待
     */
    assert(still_list != nullptr, "异常");
    const auto start_time = os::current_stamp();
    do {
        //1. 清空设置
        still_list_num = 0;
        auto current_list = still_list;
        still_list = nullptr;
        //2. 再次进行遍历
        UserThread::list().iter(iter_still_running_thread_func);

        if (still_list_num > 0) {
            //检查完毕 还是有正在运行的线程 那么我们睡眠一会进行等待
            Safepoint::back_off(start_time);
        }
        //增加一次迭代的次数
        ++iterations;

    } while (still_list_num > 0);


    //返回迭代次数
    return iterations;
}


void Safepoint::back_off(ticks_t start_time) {
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
