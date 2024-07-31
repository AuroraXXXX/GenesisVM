//
// Created by aurora on 2024/7/16.
//

#ifndef KERNEL_UTILS_LOCKER_HPP
#define KERNEL_UTILS_LOCKER_HPP


#include "plat/mem/allocation.hpp"
#include "plat/thread/Mutex.hpp"
#include "plat/thread/Monitor.hpp"
/**
 * 锁的工具类
 */
class MutexLocker : public StackObject {
private:
    Mutex *const _mutex;
    NONCOPYABLE(MutexLocker);

public:
    explicit MutexLocker(Mutex *mutex) : _mutex(mutex) {
        if (mutex == nullptr) {
            return;
        }
        this->_mutex->lock();
    };

    ~MutexLocker() {
        if (this->_mutex == nullptr) {
            return;
        }
        this->_mutex->unlock();
    };

};
class MonitorLocker : public StackObject {
private:
    Monitor *const _monitor;
    NONCOPYABLE(MonitorLocker);

public:
    inline  explicit MonitorLocker(Monitor *monitor) : _monitor(monitor) {
        assert(monitor != nullptr, "mutex 对象不为空");
        this->_monitor->lock();
    };

     inline ~MonitorLocker() {
        this->_monitor->unlock();
    };
    /**
     * 等待
     * @param millis 0表示永久等待
     * @return false 表示超时
     */
    inline auto wait(ticks_t millis = 0) {
        OSReturn osReturn;
        OrderAccess::fence();
        osReturn = this->_monitor->wait(millis);
        OrderAccess::fence();
        return osReturn == OSReturn::OK;
    };

    inline void notify() {
        OrderAccess::fence();
        this->_monitor->notify();
        OrderAccess::fence();
    };

    inline void notify_all(){
        OrderAccess::fence();
        this->_monitor->notify_all();
        OrderAccess::fence();
    };
};
#endif //KERNEL_UTILS_LOCKER_HPP
