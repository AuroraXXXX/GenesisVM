//
// Created by aurora on 2024/6/29.
//

#ifndef PLATFORM_GLOBAL_LOCKER_HPP
#define PLATFORM_GLOBAL_LOCKER_HPP
#include "platform/allocation.hpp"
/**
 * 全局的锁
 */
class GlobalLocker :public StackObject{
public:
    explicit GlobalLocker();

    ~GlobalLocker();
};


#endif //PLATFORM_GLOBAL_LOCKER_HPP
