//
// Created by aurora on 2024/8/6.
//

#ifndef GENESISVM_BIT_HPP
#define GENESISVM_BIT_HPP
#include <concepts>
#include "stdtype.hpp"
#include "plat/constants.hpp"
#include "plat/utils/robust.hpp"
class Bit {
public:
    /**
     * 计算数据中1的数量
     * @tparam T
     * @param value
     * @return
     */
    template<std::integral T>
    requires (sizeof(T) == 4)
    inline static int count_total_ones(T value) {
        return __builtin_popcount(value);
    }

    template<std::integral T>
    requires (sizeof(T) == 8)
    inline static int count_total_ones(T value) {
        return __builtin_popcountll(value);
    }

    /**
     * 从低位开始计算 第一个1 的序号
     * 从0开始计算
     * @tparam T
     * @param value
     * @return -1 表示未找到
     */
    template<std::integral T>
    requires (sizeof(T) == 4)
    inline static int offset_right_one(T value) {
        return __builtin_ffs(value) - 1;
    }

    template<std::integral T>
    requires (sizeof(T) == 8)
    inline static int offset_right_one(T value) {
        return __builtin_ffsll(value) - 1;
    }

    /**
     * 从二进制最高位向最低位 计算连续0的个数
     * @tparam T 类型
     * @param value
     * @return
     */
    template<std::integral T>
    requires (sizeof(T) == 4)
    inline static int count_left_zero(T value) {
        return __builtin_clz(value);
    }

    template<std::integral T>
    requires (sizeof(T) == 8)
    inline static int count_left_zero(T value) {
        return __builtin_clzll(value);
    }

    /**
     * 从二进制最低位向最高位 计算连续0的个数
     * @tparam T 类型
     * @param value
     * @return
     */
    template<std::integral T>
    inline static int count_right_zero(T value) {
        return __builtin_ctz(value);
    }

    template<std::integral T>
    requires (sizeof(T) == 8)
    inline static int count_right_zero(T value) {
        return __builtin_ctzll(value);
    }

    /**
     * 将第n位置为1
     * @tparam T 必须是基本类型
     * @param x
     * @param n
     */
    template<std::integral T>
    inline static void bits_set_nth(T &x, size_t n) {
        assert(n < sizeof(T) * BitsPerByte, "index is out of boundary");
        x |= (T) 1 << n;
    };


    /**
     * 清除第n位的标记
     * @tparam T
     * @param x
     * @param n 需要清楚比特位编号
     */
    template<std::integral T>
    inline static void bit_clear_nth(T &x, size_t n) {
        assert(n < sizeof(T) * BitsPerByte, "index is out of boundary");
        x &= ~((T) 1 << n);
    };

    /**
     * 判断第n位置是否被置为1
     * @tparam T 类型
     * @param x 源数据
     * @param n 需要判断比特位编号
     * @return
     */
    template<std::integral T>
    inline static bool bit_is_set_nth(T x, size_t n) {
        assert(n < sizeof(T) * BitsPerByte, "index is out of boundary");
        return (x & ((T) 1 << n)) != 0;
    };
};

#endif //GENESISVM_BIT_HPP
