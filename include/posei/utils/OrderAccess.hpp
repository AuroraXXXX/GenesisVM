//
// Created by aurora on 2023/1/2.
//

#ifndef PLATFORM_UTILS_ORDER_ACCESS_HPP
#define PLATFORM_UTILS_ORDER_ACCESS_HPP

#include "posei/init/gcc_builtin.hpp"

/**
 * 定义内存的访问顺序
 */
class OrderAccess {
public:
    /**
     * gcc的编译屏障
     * 保证编译代码时候 屏障之前的代码不会优化在屏障之后
     * 但是无法保证代码在执行时候也是这个样子
     */
    static inline void compile_barrier() {
        posei::order_access_compile_barrier();
    };

    /**
     * 既阻止GCC重排序 又阻止 CPU重排序
     *
     */
    static inline void fence() {
        posei::order_access_fence();
    };
};


#endif //PLATFORM_UTILS_ORDER_ACCESS_HPP
