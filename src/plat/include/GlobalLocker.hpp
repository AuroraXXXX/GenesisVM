//
// Created by aurora on 2024/6/29.
//

#ifndef PLAT_GLOBAL_LOCKER_HPP
#define PLAT_GLOBAL_LOCKER_HPP
#include "plat/mem/allocation.hpp"
/**
 * 全局的锁
 */
class GlobalLocker :public StackObject{
public:
    explicit GlobalLocker();

    ~GlobalLocker();
};


#endif //PLAT_GLOBAL_LOCKER_HPP
