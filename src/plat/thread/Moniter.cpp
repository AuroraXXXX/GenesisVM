//
// Created by aurora on 2023/10/21.
//

#include "plat/thread/Monitor.hpp"
#include <ctime>
#include "plat/constants.hpp"
#include "plat/thread/ThreadStatusTrans.hpp"
#include "plat/thread/OSThread.hpp"
#include "iostream"
OSReturn Monitor::wait(ticks_t millis) {
    if (millis == 0) {
        //表示无限期的等待了
        ThreadStatusBlockedTrans blocked;
        this->set_owner(nullptr);
        //下面是要进行等待的 内部实际上会释放的锁的 所以此处也需要将持有者设置为空
        int32_t status = ::pthread_cond_wait(&this->_cond, &this->_mutex);
        //说明是被唤醒的 所以说可以认为是持有锁的
        this->set_owner(OSThread::current());
        assert(status == 0 || status == ETIMEDOUT, "cond_wait");
        return OSReturn::OK;
    } else {
        //表示需要限时等待
        timespec spec{0,0};
        clock_gettime(CLOCK_REALTIME, &spec);

        // 将时间加上去
        constexpr auto ms_per_sec = TicksPerS / TicksPerMS;
        constexpr auto ns_per_ms = TicksPerMS / TicksPerNS;
        auto sec = millis / ms_per_sec;
        auto nsec = (millis - sec * ms_per_sec) * ns_per_ms;

        spec.tv_sec += (long) sec;
        spec.tv_nsec += (long) nsec;
        // 处理 tv_nsec 溢出
        if (spec.tv_nsec >= TicksPerS) {
            spec.tv_sec += spec.tv_nsec / TicksPerS;
            spec.tv_nsec %= TicksPerS;
        }
        ThreadStatusBlockedTrans blocked;
        //下面是要进行等待的 内部实际上会释放的锁的 所以此处也需要将持有者设置为空
        this->set_owner(nullptr);
        OrderAccess::compile_barrier();
        int32_t status = ::pthread_cond_timedwait(&this->_cond, &this->_mutex, &spec);
        OrderAccess::compile_barrier();
        //说明是被唤醒的 所以说可以认为是持有锁的
        this->set_owner(OSThread::current());
        assert(status == 0 || status == ETIMEDOUT, "cond_timewait %d",status);
        return status == 0 ? OSReturn::OK : OSReturn::TIMEOUT;
    }
}

Monitor::Monitor(const char *name, bool recursive) noexcept :
        Mutex(name, recursive),
        _cond(PTHREAD_COND_INITIALIZER) {
}
