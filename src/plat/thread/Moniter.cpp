//
// Created by aurora on 2023/10/21.
//

#include "plat/thread/Monitor.hpp"
#include <ctime>
#include "plat/constants.hpp"
#include "plat/thread/ThreadStatusTrans.hpp"
#include "iostream"
OSReturn Monitor::wait(ticks_t millis) {
    if (millis == 0) {
        //表示无限期的等待了
        ThreadStatusBlockedTrans blocked;
        int32_t status = pthread_cond_wait(&this->_cond, &this->_mutex);
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
        int32_t status = pthread_cond_timedwait(&this->_cond, &this->_mutex, &spec);
        assert(status == 0 || status == ETIMEDOUT, "cond_timewait %d",status);
        return status == 0 ? OSReturn::OK : OSReturn::TIMEOUT;
    }
}

Monitor::Monitor(const char *name, bool recursive) :
        Mutex(name, recursive),
        _cond(PTHREAD_COND_INITIALIZER) {
}
