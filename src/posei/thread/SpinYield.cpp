// Created by aurora on 2023/1/22.
//

#include "plat/thread/SpinYield.hpp"
#include "plat/os/cpu.hpp"
#include "plat/os/time.hpp"
#include "plat/stream/CharOStream.hpp"
#include "plat/utils/robust.hpp"
#include <sched.h>
#include <ctime>

int SpinYield::spin_pause() {
    return -1;
}

void SpinYield::yield_or_sleep() {
    if (this->_yields < this->_yield_limit) {
        ++this->_yields;
        //让出CPU
        ::sched_yield();
    } else {
        this->_sleep_ticks += SpinYield::sleep(this->_per_sleep_ns);;
    }
}

SpinYield::SpinYield(uint32_t spin_limit, uint32_t yield_limit, uint32_t per_sleep_ns) :
        _sleep_ticks(0),
        _spins(0),
        _yields(0),
        _spin_limit(os::is_MP() ? spin_limit : 0),
        _yield_limit(yield_limit),
        _per_sleep_ns(per_sleep_ns) {
}

void SpinYield::wait() {
    if (this->_spins < this->_spin_limit) {
        ++this->_spins;
        SpinYield::spin_pause();
    } else {
        this->yield_or_sleep();
    }
}

void SpinYield::report(CharOStream *out) const {
    bool waiting = false;
    if (_spins) {
        out->print("spins = %u, ", _spins);
        waiting = true;
    }
    if (_yields) {
        out->print("yields = %u, ", _yields);
        waiting = true;
    }
    if (_sleep_ticks) {
        out->print("sleep = %u usecs", _sleep_ticks / 1000);
        waiting = true;
    }
    if (!waiting) {
        out->print("no waiting");
    }
}

ticks_t SpinYield::sleep(uint32_t ns) {
    assert(ns < TicksPerS, "The spin sleep time is too long, up to 1s");
    const auto start_ticks = os::current_stamp();
    struct timespec spec{
            0,
            ns
    };
    ::nanosleep(&spec, nullptr);
    return (os::current_stamp() - start_ticks);
}
