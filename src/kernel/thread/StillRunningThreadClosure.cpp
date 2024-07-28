//
// Created by aurora on 2024/7/17.
//
#include "StillRunningThreadClosure.hpp"
#include "kernel/thread/LangThread.hpp"

StillRunningThreadClosure::StillRunningThreadClosure() :
        _still_list(nullptr),
        _still_num(0) {}

void StillRunningThreadClosure::do_thread(PlatThread *thread) {
    if (!thread->is_running_state()) {
        //线程没有在运行了 就减少统计信息
        return;
    }
    const auto lang_thread =  (LangThread*)thread;
    lang_thread->_still_running_next  = this->_still_list;
    this->_still_list = lang_thread;
    ++this->_still_num;
}

void StillRunningThreadClosure::clear() {
    this->_still_list = nullptr;
    this->_still_num = 0;
}
