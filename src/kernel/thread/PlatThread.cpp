//
// Created by aurora on 2024/3/3.
//
#include "kernel/thread/PlatThread.hpp"
#include "plat/utils/OrderAccess.hpp"
#include "kernel_mutex.hpp"
PlatThread *volatile PlatThread::_user_thread_list = nullptr;
PlatThread *volatile PlatThread::_daemon_thread_list = nullptr;
void PlatThread::add_to_list() {
    PlatThread *volatile *list_ptr;
    if (this->is_user_thread()) {
        list_ptr = &PlatThread::_user_thread_list;
    } else if (this->is_daemon_thread()) {
        list_ptr = &PlatThread::_daemon_thread_list;
    } else {
        guarantee(false, "user thread type and daemon thread type is error");
        return;
    }
    OrderAccess::store<PlatThread *>(&this->_next, *list_ptr);
    OrderAccess::store<PlatThread *>(list_ptr, this);
}

void PlatThread::remove_from_list() {

    PlatThread *volatile *list_ptr;
    if (this->is_user_thread()) {
        list_ptr = &PlatThread::_user_thread_list;
    } else if (this->is_daemon_thread()) {
        list_ptr = &PlatThread::_daemon_thread_list;
    } else {
        guarantee(false, "user thread type and daemon thread type is error");
        return;
    }

    PlatThread *prev = nullptr;
    PlatThread *cur = OrderAccess::load(list_ptr);
    while (cur != nullptr) {
        auto next = OrderAccess::load(&cur->_next);
        if (cur == this) {
            if (prev == nullptr) {
                OrderAccess::store(list_ptr, next);
            } else {
                OrderAccess::store<PlatThread *>(&this->_next, next);
            }
            break;
        }
        prev = cur;
        cur = next;
    }
}


void PlatThread::thread_do(
        PlatThread * volatile* list_ptr,
        ThreadClosure *closure) {
    auto cur = OrderAccess::load(list_ptr);
    while (cur != nullptr) {
        closure->do_thread(cur);
        cur = OrderAccess::load(&cur->_next);
    }
}

PlatThread::PlatThread() :
        OSThread(),
        _next(nullptr) {

}

void PlatThread::pre_run() {
    assert(this->is_daemon_thread() ^ this->is_user_thread(), "Thread类型错误");
    auto lock = this->is_daemon_thread() ? LangThreadList_lock : NonLangThreadList_lock;
    MutexLocker locker(lock);
    this->add_to_list();
}

void PlatThread::post_run() {
    assert(this->is_daemon_thread() ^ this->is_user_thread(), "Thread类型错误");
    auto lock = this->is_daemon_thread() ? LangThreadList_lock : NonLangThreadList_lock;
    MutexLocker locker(lock);
    this->remove_from_list();
}
