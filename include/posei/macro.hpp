//
// Created by aurora on 2023/12/27.
//

#ifndef PLATFORM_MACRO_HPP
#define PLATFORM_MACRO_HPP
#define offset_of(klass, field) (size_t)((int64_t)&(((klass*)16)->field) - 16)

/**
 * 不允许类的对象发生移动 拷贝等
 */
#define NONCOPYABLE(C) C(C const&) = delete; C& operator=(C const&) = delete

#define ALWAYS_NOT_INLINE __attribute__((noinline))
#ifndef BUILD_TYPE_TRACE
#define BUILD_TYPE_TRACE
#endif
/**
 * 这样的设计 用于加速代码的执行 和兼顾开发的效率
 * BUILD_TYPE_TRACE 宏 用于开启debug级别的信息输出
 * BUILD_TYPE_DEBUG 宏 用语开启trace级别的信息输出
 */
#ifdef BUILD_TYPE_TRACE
#define DEBUG_MODE_ONLY(code) code
#define TRACE_MODE_ONLY(code) code
//诊断代码的标志
#define DIAGNOSE
#elif BUILD_TYPE_DEBUG
#define DEBUG_MODE_ONLY(code) code
#define TRACE_MODE_ONLY(code)
#else
#define DEBUG_MODE_ONLY(code)
#define TRACE_MODE_ONLY(code)
#endif

#ifdef DEVELOP
#define DEVELOP_ONLY(code) code
#else
#define DEVELOP_ONLY(code)
#endif


template<typename T>
inline T MAX2(T a, T b) { return (a > b) ? a : b; }

template<typename T>
inline T MIN2(T a, T b) { return (a < b) ? a : b; }

template<typename T>
inline T MIN3(T a, T b, T c) { return MIN2(MIN2(a, b), c); }

template<typename T>
inline T MAX3(T a, T b, T c) { return MAX2(MAX2(a, b), c); }

/**
 * 将value限制在min 和max之间
 * @tparam T
 * @param value
 * @param min
 * @param max
 * @return
 */
template<typename T>
inline T clamp(T value, T min, T max) {
    return MIN2<T>(MAX2<T>(value, min), max);
}

template<typename T>
inline bool is_clamp(T value, T min, T max) {
    return value >= min && value <= max;
}



/**
 * 获取调用者的当前的指令地址
 * @tparam level
 * @return
 */
inline void *current_thread_pc() {
    return __builtin_return_address(0);
}

/**
 * 获取调用者堆栈信息
 * @tparam depth 0-～
 * @return
 */
template<int depth>
void *return_thread_pc() {
    return __builtin_return_address(depth);
}

/**
 * ---------------------
 * 格式化的字符串
 * ---------------------
 */
#define PTR_FORMAT "0x%016lx"
#define SIZE_FORMAT "%lu"
#define LONG_FORMAT "%ld"
#define INT_FORMAT "%d"
#define UINT_FORMAT "%u"
#define UINTX_FORMAT "%lu"
#define INTX_FORMAT "%ld"

#include "linux/version.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)
#error "Linux kernel version must be 3.11.0 or later"
#endif
#endif //PLATFORM_MACRO_HPP
