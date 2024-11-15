//
// Created by aurora on 2024/11/14.
//

#ifndef GENESISVM_GCC_BUILTIN_HPP
#define GENESISVM_GCC_BUILTIN_HPP
#define ALWAYS_NOT_INLINE __attribute__((noinline))

namespace posei {
    /**
     * 获取调用者的信息
     * @return
     */
    ALWAYS_NOT_INLINE void *current_thread_pc() {
        return __builtin_return_address(0);
    }

    template<int32_t depth = 0>
    ALWAYS_NOT_INLINE void* return_thread_pc(){
        return __builtin_return_address(depth);
    }
    /**
     * 计算数据中1的数量
     * @tparam T
     * @param value
     * @return
     */
    template<typename T>
    requires (sizeof(T) == 4 || sizeof(T) == 8)
    inline static int count_total_ones(T value) {
        if constexpr (sizeof(T) == 4) {
            return __builtin_popcount(value);
        } else {
            return __builtin_popcountll(value);
        }
    }

    /**
     * 从低位开始计算 第一个1 的序号
     * 从0开始计算
     * @tparam T
     * @param value
     * @return -1 表示未找到
     */
    template<typename T>
    requires (sizeof(T) == 4 || sizeof(T) == 8)
    inline static int offset_right_one(T value) {
        if constexpr (sizeof(T) == 4) {
            return __builtin_ffs(value) - 1;
        } else {
            return __builtin_ffsll(value) - 1;
        }
    }

    /**
     * 从二进制最高位向最低位 计算连续0的个数
     * @tparam T 类型
     * @param value
     * @return
     */
    template<typename T>
    requires (sizeof(T) == 4 || sizeof(T) == 8)
    inline static int count_left_zero(T value) {
        if constexpr (sizeof(T) == 4) {
            return __builtin_clz(value);
        } else {
            return __builtin_clzll(value);
        }
    }

    /**
     * 从二进制最低位向最高位 计算连续0的个数
     * @tparam T 类型
     * @param value
     * @return
     */
    template<typename T>
    inline static int count_right_zero(T value) {
        if constexpr (sizeof(T) == 4) {
            return __builtin_ctz(value);
        } else {
            return __builtin_ctzll(value);
        }
    }

    /**
     * gcc的编译屏障
     * 保证编译代码时候 屏障之前的代码不会优化在屏障之后
     * 但是无法保证代码在执行时候也是这个样子
     */
    inline void order_access_compile_barrier() {
        __asm__ volatile( "" : : : "memory");
    }

    inline void order_access_fence() {
        __sync_synchronize();
        order_access_compile_barrier();
    }

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
    inline T atomic_cas(T volatile *ptr, T old_val, T new_val) {
        return __sync_val_compare_and_swap(ptr, old_val, new_val);
    }

    /**
     * 将ptr指向的值返回，并原子性的将new_val赋值给ptr指向的内存
     * @tparam T
     * @param ptr
     * @param new_val
     * @return
     */
    template<typename T>
    inline T atomic_xchg(T volatile *ptr, T new_val) {
        return __sync_lock_test_and_set(ptr, new_val);
    }

    /**
     * 将value加到*ptr上，结果更新到*ptr，并返回操作之前*ptr的值
     * @tparam T
     * @param ptr
     * @param val
     * @return
     */
    template<typename T>
    inline T atomic_fetch_and_add(volatile T *ptr, T val) {
        return __sync_fetch_and_add(ptr, val);
    }

    template<typename T>
    inline T atomic_fetch_and_sub(volatile T *ptr, T val) {
        return __sync_fetch_and_sub(ptr, val);
    }
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
}

#endif //GENESISVM_GCC_BUILTIN_HPP
