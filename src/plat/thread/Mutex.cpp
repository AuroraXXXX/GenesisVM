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
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    if (recursive) {
        ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    } else {
        ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    }
    ::pthread_mutex_init(&this->_mutex, &attr);
    ::pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex() {
    ::pthread_mutex_destroy(&this->_mutex);
}

bool Mutex::owned_by_self() const {
    return OSThread::current() == this->owner() && this->owner() != nullptr;
}

bool Mutex::try_lock() {
    auto status = ::pthread_mutex_trylock(&this->_mutex);
    const auto success = status == 0;
    assert(success || status == EBUSY, "pthread_mutex_trylock");
    if (success) {
        assert(!this->is_locked() || this->owned_by_self(), "mutex owner设置错误");
        //is locked
        if(!is_locked()){
            this->set_owner(OSThread::current());
        }
    }
    return success;
}

void Mutex::lock() {
    //
    int32_t status ;
    {
        ThreadStatusBlockedTrans blocked;
        status = ::pthread_mutex_lock(&this->_mutex);
    }
    assert(status == 0, "pthread_mutex_lock");
    OrderAccess::compile_barrier();
  //  assert(!this->is_locked() || this->owned_by_self(), "mutex owner设置错误 %x %x",this->owner(),OSThread::current());
    if(!(!this->is_locked() || this->owned_by_self())){
        guarantee(false,"??");
    }
    //is locked
    if(!is_locked()){
        this->set_owner(OSThread::current());
    }

}

void Mutex::unlock() {
    assert(this->owned_by_self(), "check");
    this->set_owner(nullptr);
    OrderAccess::compile_barrier();
    auto status = ::pthread_mutex_unlock(&this->_mutex);
    assert(status == 0, "pthread_mutex_unlock");
}

void Mutex::print_on(CharOStream *stream) {
    stream->print("Mutex: [" PTR_FORMAT "] %s - owner: " PTR_FORMAT,
                  this,
                  this->_name,
                  this->owner());
}


