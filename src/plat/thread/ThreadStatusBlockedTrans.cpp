//
// Created by aurora on 2024/6/28.
//
#include "plat/thread/ThreadStatusTrans.hpp"
#include "plat/thread/OSThread.hpp"

ThreadStatusBlockedTrans::ThreadStatusBlockedTrans() {
    this->_self = OSThread::current();
    assert(this->_self->state() == OSThread::STATE_RUNNING, "check");
    this->_self->tans_state(OSThread::STATE_BLOCKED);
    OrderAccess::compile_barrier();
}

ThreadStatusBlockedTrans::~ThreadStatusBlockedTrans() {
    OrderAccess::compile_barrier();
    assert(this->_self->state() == OSThread::STATE_BLOCKED, "check");
    this->_self->tans_state(OSThread::STATE_RUNNING);
}
