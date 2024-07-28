//
// Created by aurora on 2023/1/10.
//

#ifndef PLATFORM_ALIGN_HPP
#define PLATFORM_ALIGN_HPP

#include "plat/constants.hpp"
#include "stdtype.hpp"
#include "plat/utils/robust.hpp"

/**
 * 判断1个数是否是2^n
 * @tparam T 类型
 * @param value 值
 * @return
 */
template<typename T>
constexpr bool is_power_of_2(T value) {
    return value > T(0) && (value & (value - 1)) == T(0);
}

/**
 * 向下对齐，注意可能返回的值为0
 * @tparam T
 * @param val 需要对齐的值
 * @param align 对齐的粒度
 * @return 对齐后的值
 */
template<typename T>
inline constexpr T align_down(T val, size_t align) {
    assert(is_power_of_2(align), "align 不是2的幂次");
    return (T) ((size_t(val)) & ~(align - 1L));
}

/**
 * 向上对齐
 * @tparam T
 * @param val
 * @param align
 * @return
 */
template<typename T>
inline constexpr T align_up(T val, size_t align) {
    assert(is_power_of_2(align), "align 不是2的幂次");
    return (T) ((size_t(val) + (align - 1L)) & ~(align - 1L));
}

/**
 * 判断 val 是否按照align进行了对齐
 * @tparam T 类型
 * @param val
 * @param align 对齐的宽度
 * @return
 */
template<typename T>
inline bool is_aligned(T val, size_t align) {
    assert(is_power_of_2(align), "align 不是2的幂次");
    return ((size_t) val & (align - 1)) == 0;
}

/**
 * 向下对齐 但是至少是 align 大小
 * @tparam T
 * @param size
 * @param align
 * @return
 */
template<typename T>
inline constexpr T align_down_bounded(T size, size_t align) {
    auto aligned_size = align_down(size, align);
    return (aligned_size > 0) ? aligned_size : (T) align;
}

/**
 * 用于 判断是否对齐
 * @tparam T
 * @param bytes
 * @param align
 */
template<typename T>
inline void assert_is_aligned(T bytes, size_t align) {
    assert(is_aligned<T>(bytes, align),
           UINTX_FORMAT " 没有按照 " UINTX_FORMAT "字节对齐",bytes, align);
}

#endif //PLATFORM_ALIGN_HPP
