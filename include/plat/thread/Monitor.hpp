//
// Created by aurora on 2023/10/21.
//

#ifndef PLATFORM_MONITOR_HPP
#define PLATFORM_MONITOR_HPP

#include "Mutex.hpp"
#include "plat/constants.hpp"
/**
 * 支持语言层面的Object的唤醒和等待
 */
class Monitor : public Mutex {
protected:
    pthread_cond_t _cond;
public:
    explicit Monitor(const char *name, bool recursive = true) noexcept;

    /**
     * 在这个点进行等待
     * 必须在持有锁锁定的情况下
     * @param millis 超时等待的约束
     * 当0时，表示无限期等待
     *
     */
    OSReturn wait(ticks_t millis);

    /**
     * 唤醒1个等待在此处的线程
     * 必须在持有锁锁定的情况下
     */
    void notify() {
        auto status = ::pthread_cond_signal(&this->_cond);
        assert(status == 0, "pthread_cond_signal");
    };

    /**
     * 唤醒所有等待在此处的线程
     * 必须在持有锁锁定的情况下
     */
    void notify_all() {
        auto status = ::pthread_cond_broadcast(&this->_cond);
        assert(status == 0, "pthread_cond_broadcast");
    };
};


#endif //PLATFORM_MONITOR_HPP
