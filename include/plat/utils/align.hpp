//
// Created by aurora on 2023/1/10.
//

#ifndef PLATFORM_ALIGN_HPP
#define PLATFORM_ALIGN_HPP

#include "plat/utils/Bit.hpp"
#include <concepts>
#include <limits>

/**
 * 判断1个数是否是2^n
 * @tparam T 类型
 * @param value 值
 * @return
 */
template<std::integral T>
inline constexpr bool is_power_of_2(T value) {
    return value > T(0) && (value & (value - 1)) == T(0);
}


template<std::integral T>
inline constexpr bool max_power_2(){
    T max_val = std::numeric_limits<T>::max();
    return max_val - (max_val >> 1);
}

/**
  * 对数值value进行log2计算
  * 对于不是2^n的，采用截断value()
  * @tparam T
  * @param value
  * @return
  */
template<std::integral T>
inline int log2i(T value) {
    assert(value > T(0), "value must be > 0");
    constexpr int bits = sizeof(value) * BitsPerByte;
    return bits - Bit::count_left_zero(value) - 1;
}

/**
 * 相比于log2i函数 当输入0时候 返回-1
 * @tparam T
 * @param value
 * @return
 */
template<std::integral T>
inline int log2i_graceful(T value) {
    if (value == 0) {
        return -1;
    }
    constexpr int bits = sizeof(value) * BitsPerByte;
    return bits - Bit::count_left_zero<T>(value) - 1;
}

/**
 * 对value进行log计算 但是这个数本身必须满足 2^n
 * @tparam T
 * @param value
 * @return
 */
template<std::integral T>
inline constexpr int log2i_exact(T value) {
    assert(is_power_of_2<T>(value),
           "value must be a power of 2: "
                   UINTX_FORMAT,
           static_cast<uint64_t>(value));
    return (int32_t) Bit::count_right_zero(value);
};

/**
 * 将value向下对齐
 * @tparam T
 * @param value
 * @return
 */
template<std::integral T>
inline T round_down_power_of_2(T value) {
    assert(value > 0, "Invalid value");
    return T(1) << log2i(value);
}

/**
 * 向上进行2^n对齐
 * @tparam T
 * @param value
 * @return
 */
template<std::integral T>
inline T round_up_power_of_2(T value) {
    assert(value > 0, "Invalid value");
    assert(value <= max_power_2<T>(), "Overflowing maximum allowed power of two with "
            UINTX_FORMAT,
           static_cast<uint64_t>(value));
    if (is_power_of_2(value)) {
        return value;
    }
    return T(1) << (log2i<T>(value) + 1);
}

/**
 * 向下对齐，注意可能返回的值为0
 * @tparam T
 * @param val 需要对齐的值
 * @param align 对齐的粒度
 * @return 对齐后的值
 */
template<std::integral T>
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
template<std::integral T>
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
template<std::integral T>
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
template<std::integral T>
inline void assert_is_aligned(T bytes, size_t align) {
    assert(is_aligned<T>(bytes, align),
           UINTX_FORMAT " 没有按照 " UINTX_FORMAT "字节对齐",bytes, align);
}

#endif //PLATFORM_ALIGN_HPP
