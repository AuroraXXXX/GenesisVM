//
// Created by aurora on 2023/1/13.
//

#include "platform/thread/WaitBarrier.hpp"
#include "platform/thread/ThreadStatusTrans.hpp"
#include "platform/utils/robust.hpp"
#include "platform/os.hpp"
#include "pthread.h"
#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <atomic>
WaitBarrier::~WaitBarrier() {
    assert(this->_futex_barrier == 0, "存在线程未唤醒");
}

void WaitBarrier::arm(int barrier_num) {
    assert(this->_futex_barrier == 0, "已经设置了 无法再次设置");
    this->_futex_barrier = barrier_num;
    //cpu 屏障
//    OrderAccess::fence();
}

void WaitBarrier::disarm() {
    assert(this->_futex_barrier != 0, "不应为0");
    this->_futex_barrier = 0;
    /**
     * 设置屏障 防止上面写入数据出现在下面
     */
    std::atomic_thread_fence(std::memory_order::seq_cst);
    /**
     * 唤醒所有的线程
     */
    const auto wake_num = (int) syscall(SYS_futex,
                                        &this->_futex_barrier,
                                        FUTEX_WAKE_PRIVATE,
                                        INT32_MAX,
                                        nullptr);
    guarantee(wake_num >= 0, "futex FUTEX_WAKE 失败");
}

void WaitBarrier::wait(int barrier_tag) {
    assert(barrier_tag != 0, "正在一个未设置的值上等待");
    if (barrier_tag == 0 ||
        barrier_tag != _futex_barrier) {
        return;
    }
    ThreadStatusBlockedTrans tans;
    std::atomic_thread_fence(std::memory_order::seq_cst);
    auto uaddr = &this->_futex_barrier;
    do {
        const auto state = (int) syscall(SYS_futex,
                                         &uaddr,
                                         FUTEX_WAIT_PRIVATE,
                                         barrier_tag,
                                         0);
        guarantee((state == 0) ||
                  (state == -1 && errno == EAGAIN) ||
                  (state == -1 && errno == EINTR),
                  "futex FUTEX_WAIT 失败");
        /**
         * 得到返回0 我们还需要再次进行检查 防止虚假的唤醒
         * 有些错误可以重试 我们也进行重试
         */
    } while (barrier_tag == *uaddr);
}
