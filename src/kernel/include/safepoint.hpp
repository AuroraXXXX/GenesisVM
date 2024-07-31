//
// Created by aurora on 2022/12/19.
//

#ifndef VMACHINE_SAFEPOINT_HPP
#define VMACHINE_SAFEPOINT_HPP

#include "plat/mem/allocation.hpp"
#include "kernel/utils/TimeStamp.hpp"
#include "plat/utils/OrderAccess.hpp"
#include "plat/thread/WaitBarrier.hpp"

class LangThread;


/**
 * 安全点控制
 * 用于控制所有语言层面的线程 在安全点的启停
 */
class SafepointSynchronize : public AllStatic {
    friend class LangThread;

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
    static volatile SynchronizeState _state;

    /**
     * 偶数表示 之前没有进行同步
     * 奇数表示
     */
    static volatile int32_t _safe_point_check;
    /**
     * 用于同步多个线程
     */
    static WaitBarrier _wait_barrier;
    /**
     * 开始通知线程同步的时间
     */
    static TimeStamp _beg_time;
    static TimeStamp _end_time;

    static inline auto state() {
        return OrderAccess::load<int32_t>(reinterpret_cast<volatile const int *>(&_state));
    };


    /**
     * 内核线程开始通知同步线程 并且进行等待
     * @param lang_threads 线程的个数
     * @param init_running 一开始还在运行的线程
     * @return 循环的次数
     */
    static int synchronize_threads( size_t *init_running);


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
        return SafepointSynchronize::state() == SynchronizeState::synchronized;
    };


};


#endif //VMACHINE_SAFEPOINT_HPP
