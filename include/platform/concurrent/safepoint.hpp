//
// Created by aurora on 2022/12/19.
//

#ifndef CONCURRENT_SAFEPOINT_HPP
#define CONCURRENT_SAFEPOINT_HPP

#include "platform/allocation.hpp"
#include "platform/concurrent/WaitBarrier.hpp"
#include <atomic>

/**
 * 安全点控制
 * 用于控制所有语言层面的线程（即所有UserThread） 在安全点的启停
 */
class Safepoint : public AllStatic {
    friend class OSThread;

public:
    enum SynchronizeState : int32_t {
        //线程在安全点没有同步
        not_synchronized,
        //正在线程同步
        synchronizing,
        //所有的lang 线程 全部停止 只有vm thread运行
        synchronized
    };

private:
    static std::atomic<SynchronizeState> _state;

    /**
     * 偶数表示 之前没有进行同步
     * 奇数表示
     */
    static std::atomic<int32_t> _safe_point_check;
    /**
     * 用于同步多个线程
     */
    static WaitBarrier _wait_barrier;
    /**
     * 开始通知线程同步的时间
     */
    static ticks_t _beg_time;
    static ticks_t _end_time;


    /**
     * 内核线程开始通知同步线程 并且进行等待
     * @param lang_threads 线程的个数
     * @param init_running 一开始还在运行的线程
     * @return 循环的次数
     */
    static int synchronize_threads(size_t *init_running);


    /**
     * VMThread 等待Lang线程同步时 睡眠的策略
     * @param ticks 睡眠的节拍数
     */
    static void back_off(ticks_t ticks);

public:
    /**
     * 开始安全点的同步
     */
    static void begin();

    /**
     * 结束安全地的同步
     */
    static void end();

    /**
     * 只有已经同步完成了 才能称之为安全点
     * @return
     */
    static inline bool is_at_safepoint() {
        return Safepoint::_state.load() == SynchronizeState::synchronized;
    };
    /**
     * 当前是否正在同步
     * @return
     */
    static inline bool is_synchronizing(){
        return Safepoint::_state.load() == SynchronizeState::synchronizing;
    };

    /**
     * 线程休眠在此处
     *
     */
    static void wait_on_barrier() {
         // 等待屏障，直到_safe_point_check的值为true
         _wait_barrier.wait(_safe_point_check.load());
    };

    /**
     * 当前线程是否已经同步
     * @return
     */
    static inline bool is_synchronized() {
        return Safepoint::_state.load() == SynchronizeState::synchronized;
    }
};


#endif //CONCURRENT_SAFEPOINT_HPP
