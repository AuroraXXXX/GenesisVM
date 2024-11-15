//
// Created by aurora on 2024/8/6.
//

#ifndef GENESISVM_BIT_HPP
#define GENESISVM_BIT_HPP
#include <concepts>
#include "stdtype.hpp"
#include "posei/constants.hpp"
class Bit {
public:
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
