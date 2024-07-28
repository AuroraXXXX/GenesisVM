//
// Created by aurora on 2024/1/10.
//

#ifndef PLATFORM_SEMAPHORE_HPP
#define PLATFORM_SEMAPHORE_HPP

#include "semaphore.h"
#include "plat/mem/allocation.hpp"
#include "plat/constants.hpp"


class Semaphore : public CHeapObject<MEMFLAG::Internal> {
private:
    NONCOPYABLE(Semaphore);

    sem_t _semaphore;
public:
    /**
     * 构造函数
     * @param value 信号量的值
     */
    explicit Semaphore(uint32_t value = 0);

    ~Semaphore();

    /**
     * 唤醒
     * @param count 唤醒的个数
     */
    void signal(uint32_t count = 1);

    /**
     * 休眠
     */
    void wait();

    /**
     * 尝试休眠
     * @return
     */
    bool try_wait();
};


#endif //PLATFORM_SEMAPHORE_HPP
