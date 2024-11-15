//
// Created by aurora on 2024/11/15.
//

#ifndef GENESISVM_ATOMIC_HPP
#define GENESISVM_ATOMIC_HPP

#include "posei/init/gcc_builtin.hpp"

class Atomic {
public:
    /**
     * CAS 操作
     * 当ptr指向的变量 的数值 等于 old_val 时
     * 我们使用 new_val 进行了修改
     * 返回修改前ptr指向变量的数值
     *
     * 当ptr指向的变量 的数值 不等于 old_val 时
     * 仅仅读取ptr指向变量的数值
     *
     * volatile用来后置表示每次都要从内存读这个指针
     * @tparam T 类型
     * @param ptr 变量指针
     * @param old_val 旧的值
     * @param new_val 新的数值
     * @return ptr 最新一次读取的值
     */
    template<typename T>
    static inline T cas(T volatile *ptr, T old_val, T new_val) {
        return posei::atomic_cas(ptr, old_val, new_val);
    }

    /**
     * 将ptr指向的值返回，并原子性的将new_val赋值给ptr指向的内存
     * @tparam T
     * @param ptr
     * @param new_val
     * @return
     */
    template<typename T>
    inline T xchg(T volatile *ptr, T new_val) {
        return posei::atomic_xchg(ptr, new_val);
    }

    /**
     * 将value加到*ptr上，结果更新到*ptr，并返回操作之前*ptr的值
     * @tparam T
     * @param ptr
     * @param val
     * @return
     */
    template<typename T>
    inline T fetch_and_add(volatile T *ptr, T val) {
        return posei::atomic_fetch_and_add(ptr, val);
    }

    template<typename T>
    inline T fetch_and_sub(volatile T *ptr, T val) {
        return posei::atomic_fetch_and_sub(ptr, val);
    }
};

#endif //GENESISVM_ATOMIC_HPP
