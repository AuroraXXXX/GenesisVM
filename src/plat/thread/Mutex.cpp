//
// Created by aurora on 2023/10/21.
//

#include "plat/thread/Mutex.hpp"
#include "plat/thread/OSThread.hpp"
#include "plat/thread/ThreadStatusTrans.hpp"
#include "plat/stream/CharOStream.hpp"
Mutex::Mutex(
        const char *name,
        bool recursive) noexcept:
        _name(name),
        _owner(nullptr) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    if (recursive) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    } else {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    }
    pthread_mutex_init(&this->_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex() {
    pthread_mutex_destroy(&this->_mutex);
}

bool Mutex::owned_by_self() {
    return OSThread::current() == this->_owner;
}

bool Mutex::try_lock() {
    auto thread = OSThread::current();
    auto status = pthread_mutex_trylock(&this->_mutex);
    const auto success = status == 0;
    assert(success || status == EBUSY, "pthread_mutex_trylock");
    if (success) {
        assert(this->owner() == nullptr, "mutex owner设置错误");
        this->_owner = thread;
    }
    return success;
}

void Mutex::lock() {
    //
    const auto current = OSThread::current();
    int32_t status ;
    {
        ThreadStatusBlockedTrans blocked;
        status = pthread_mutex_lock(&this->_mutex);
    }
    assert(this->owner() == nullptr, "mutex owner设置错误");

    this->_owner = current;
    assert(status == 0, "pthread_mutex_lock");
}

void Mutex::unlock() {
    auto current = OSThread::current();
    assert(this->_owner == current, "check");
    this->_owner = nullptr;
    auto status = pthread_mutex_unlock(&this->_mutex);
    assert(status == 0, "pthread_mutex_unlock");
}

void Mutex::print_on(CharOStream *stream) {
    stream->print("Mutex: [" PTR_FORMAT "] %s - owner: " PTR_FORMAT,
                  this,
                  this->_name,
                  this->_owner);
}


