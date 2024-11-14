//
// Created by aurora on 2023/1/13.
//

#include "plat/thread/WaitBarrier.hpp"
#include "plat/thread/ThreadStatusTrans.hpp"
#include "plat/utils/OrderAccess.hpp"
#include "plat/utils/robust.hpp"
#include "inner_os.hpp"
WaitBarrier::~WaitBarrier() {
    assert(this->_futex_barrier == 0, "存在线程未唤醒");
}

void WaitBarrier::arm(int barrier_num) {
    assert(this->_futex_barrier == 0, "已经设置了 无法再次设置");
    this->_futex_barrier = barrier_num;
    //cpu 屏障
    OrderAccess::fence();
}

void WaitBarrier::disarm() {
    assert(this->_futex_barrier != 0, "不应为0");
    this->_futex_barrier = 0;
    OrderAccess::fence();
    os::wakeup(const_cast<int *>(&_futex_barrier), INT32_MAX);
}

void WaitBarrier::wait(int barrier_tag) {
    assert(barrier_tag != 0, "正在一个未设置的值上等待");
    if (barrier_tag == 0 ||
        barrier_tag != _futex_barrier) {
        OrderAccess::fence();
        return;
    }
    ThreadStatusBlockedTrans tans;
    OrderAccess::compile_barrier();
    os::suspend((int *) &_futex_barrier, barrier_tag);
}
