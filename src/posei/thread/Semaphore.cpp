//
// Created by aurora on 2024/1/10.
//

#include "plat/thread/Semaphore.hpp"
#include "plat/utils/robust.hpp"
#include <cerrno>
#include "plat/thread/ThreadStatusTrans.hpp"
Semaphore::Semaphore(uint32_t value) {
    auto ret = ::sem_init(&this->_semaphore, 0, value);
    guarantee(ret == 0, "sem_init failed");
}

Semaphore::~Semaphore() {
    auto ret = ::sem_destroy(&this->_semaphore);
    assert(ret == 0, "sem_destroy failed");
}

void Semaphore::signal(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        auto ret = ::sem_post(&this->_semaphore);
        assert(ret ==0,"sem_post failed");
    }
}

void Semaphore::wait() {
    int32_t ret;
    ThreadStatusBlockedTrans blocked;
    do{
        ret = ::sem_wait(&this->_semaphore);
    } while (ret != 0 && errno == EINTR);
    assert(ret == 0,"sem_wait failed");
}

bool Semaphore::try_wait() {
    int32_t ret;
    do{
        ret = ::sem_trywait(&this->_semaphore);
    } while (ret != 0 && errno == EINTR);
    assert(ret == 0 || errno == EAGAIN,"sem_wait failed");
    return ret == 0;
}



