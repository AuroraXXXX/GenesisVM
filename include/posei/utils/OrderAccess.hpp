//
// Created by aurora on 2023/1/2.
//

#ifndef PLATFORM_UTILS_ORDER_ACCESS_HPP
#define PLATFORM_UTILS_ORDER_ACCESS_HPP

#include <concepts>

/**
 * 定义内存的访问顺序
 */
class OrderAccess  {
public:
    /**
     * gcc的编译屏障
     * 保证编译代码时候 屏障之前的代码不会优化在屏障之后
     * 但是无法保证代码在执行时候也是这个样子
     */
    static inline void compile_barrier() {
        __asm__ volatile( "" : : : "memory");
    };

    /**
     * 既阻止GCC重排序 又阻止 CPU重排序
     *
     */
    static inline void fence() {
        __sync_synchronize();
        compile_barrier();
    };

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
     * @return
     */
    template<typename T>
    static inline T cas(T volatile *ptr, T old_val, T new_val) {
        return __sync_val_compare_and_swap(ptr, old_val, new_val);
    };
    /**
     * 将ptr指向的值返回，并原子性的将new_val赋值给ptr指向的内存
     * @tparam T
     * @param ptr
     * @param new_val
     * @return
     */
    template<typename T>
    static inline T xchg(T volatile* ptr,T new_val){
        return __sync_lock_test_and_set(ptr,new_val);
    }
    /**
     * 将value加到*ptr上，结果更新到*ptr，并返回操作之前*ptr的值
     * @tparam T
     * @param ptr
     * @param val
     * @return
     */
    template<std::integral T>
    static inline T fetch_and_add(volatile T *ptr, T val) {
        return __sync_fetch_and_add(ptr, val);
    };

    template<std::integral T>
    static inline T fetch_and_sub(volatile T *ptr, T val) {
        return __sync_fetch_and_sub(ptr, val);
    };

    template<typename T>
    static inline T load(const volatile T *ptr) {
        return *ptr;
    };

    template<typename T>
    static inline void store(volatile T *ptr, T val) {
        *ptr = val;
    };
    /**

type __sync_fetch_and_or (type *ptr, type value, ...)
// 将*ptr与value相或，结果更新到*ptr， 并返回操作之前*ptr的值
type __sync_fetch_and_and (type *ptr, type value, ...)
// 将*ptr与value相与，结果更新到*ptr，并返回操作之前*ptr的值
type __sync_fetch_and_xor (type *ptr, type value, ...)
// 将*ptr与value异或，结果更新到*ptr，并返回操作之前*ptr的值
type __sync_fetch_and_nand (type *ptr, type value, ...)
// 将*ptr取反后，与value相与，结果更新到*ptr，并返回操作之前*ptr的值
type __sync_add_and_fetch (type *ptr, type value, ...)
// 将value加到*ptr上，结果更新到*ptr，并返回操作之后新*ptr的值
type __sync_sub_and_fetch (type *ptr, type value, ...)
// 从*ptr减去value，结果更新到*ptr，并返回操作之后新*ptr的值
type __sync_or_and_fetch (type *ptr, type value, ...)
// 将*ptr与value相或， 结果更新到*ptr，并返回操作之后新*ptr的值
type __sync_and_and_fetch (type *ptr, type value, ...)
// 将*ptr与value相与，结果更新到*ptr，并返回操作之后新*ptr的值
type __sync_xor_and_fetch (type *ptr, type value, ...)
// 将*ptr与value异或，结果更新到*ptr，并返回操作之后新*ptr的值
type __sync_nand_and_fetch (type *ptr, type value, ...)
// 将*ptr取反后，与value相与，结果更新到*ptr，并返回操作之后新*ptr的值

__sync_synchronize (...)
// 发出完整内存栅栏
type __sync_lock_test_and_set (type *ptr, type value, ...)
//// 将value写入*ptr，对*ptr加锁，并返回操作之前*ptr的值。即，try spinlock语义
void __sync_lock_release (type *ptr, ...)
// 将0写入到*ptr，并对*ptr解锁。即，unlock spinlock语义
     */
};


#endif //PLATFORM_UTILS_ORDER_ACCESS_HPP
