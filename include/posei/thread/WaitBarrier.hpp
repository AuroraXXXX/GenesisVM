//
// Created by aurora on 2023/1/13.
//

#ifndef VMACHINE_WAITBARRIER_HPP
#define VMACHINE_WAITBARRIER_HPP

#include "plat/mem/allocation.hpp"

/**
 * 线程栅栏
 * 内部使用futex
 */
class WaitBarrier : public CHeapObject<MEMFLAG::Internal> {
private:
    volatile int _futex_barrier;
    NONCOPYABLE(WaitBarrier);

public:

    explicit WaitBarrier() noexcept: _futex_barrier(0) {};

    ~WaitBarrier();

    /**
     * 设置 栅栏标签
     * @param barrier_tag
     */
    void arm(int barrier_tag);

    /**
     * 唤醒全部等待在此处的线程
     */
    void disarm();

    /**
     * 仅仅当barrier_tag等于设置的标签时候
     * 线程在此等待
     * @param barrier_tag 栅栏标签
     */
    void wait(int barrier_tag);

    /**
     * 表示内部使用的何种方式实现等待栅栏
     * @return
     */
    static auto description() {
        return "futex";
    };
};


#endif //VMACHINE_WAITBARRIER_HPP
