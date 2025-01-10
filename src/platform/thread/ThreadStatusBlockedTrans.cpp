//
// Created by aurora on 2024/6/28.
//
#include "posei/thread/ThreadStatusTrans.hpp"
#include "posei/thread/OSThread.hpp"

ThreadStatusBlockedTrans::ThreadStatusBlockedTrans() {
    this->_self = OSThread::current();
    assert(this->_self->state() == OSThread::STATE_RUNNING, "check");
    this->_self->tans_state(OSThread::STATE_BLOCKED);

}

ThreadStatusBlockedTrans::~ThreadStatusBlockedTrans() {

    assert(this->_self->state() == OSThread::STATE_BLOCKED, "check");
    this->_self->tans_state(OSThread::STATE_RUNNING);
}
