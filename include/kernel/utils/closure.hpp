//
// Created by aurora on 2024/7/14.
//

#ifndef KERNEL_CLOSURE_HPP
#define KERNEL_CLOSURE_HPP
#include "plat/mem/allocation.hpp"
class Closure : public StackObject {

};

class MemoryClosure : public Closure {
public:
    /**
     * 遍历堆中内存
     * @param ptr 内存的地址
     * @return 需要移动的距离，即此次内存的实际大小
     */
    virtual size_t do_mem(void *ptr) = 0;
};

class PointerClosure : public Closure {
public:
    /**
     * 遍历指针
     * @param ptr 存储指针的地址，即二重地址，应该通过*ptr获取实际的存储的指针
     */
    virtual void do_pointer(void **ptr) = 0;
};
#endif //KERNEL_CLOSURE_HPP
