//
// Created by aurora on 2024/6/24.
//

#ifndef PLATFORM_UTILS_BYTE_ORDER_HPP
#define PLATFORM_UTILS_BYTE_ORDER_HPP

#include <endian.h>
#include "stdtype.hpp"

class ByteOrder {
public:
    /**
     * 获取网络字节序
     * @param value
     * @return
     */
    template<typename T>
    requires(sizeof(T) <= 8)
    static inline T network(T value) {
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            return htobe16(value);
        } else if constexpr (sizeof(T) == 4) {
            return htobe32(value);
        } else if constexpr (sizeof(T) == 8) {
            return htobe64(value);
        }
    };


    /**
     * 获取主机序
     * @param value
     * @return
     */
    template<typename T>
    requires(sizeof(T) <= 8)
    static inline auto host16(uint64_t value) {
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            return htole16(value);
        } else if constexpr (sizeof(T) == 4) {
            return htole32(value);
        } else if constexpr (sizeof(T) == 8) {
            return htole64(value);
        }
    };
};


#endif //PLATFORM_UTILS_BYTE_ORDER_HPP
