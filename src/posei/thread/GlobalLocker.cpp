//
// Created by aurora on 2024/6/29.
//

#include "GlobalLocker.hpp"
#include "posei/thread/Mutex.hpp"
static Mutex Global_lock("global_lock");
GlobalLocker::GlobalLocker() {
    Global_lock.lock();
}

GlobalLocker::~GlobalLocker() {
    Global_lock.unlock();
}
