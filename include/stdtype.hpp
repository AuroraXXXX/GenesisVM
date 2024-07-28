//
// Created by aurora on 2024/6/24.
//

#ifndef STDTYPE_HPP
#define STDTYPE_HPP

#include <cstdint>
/**
 * tick的类型
 */
using ticks_t = uint64_t;
using size_t = uint64_t;
static_assert(sizeof(void*) == sizeof(size_t));
#endif //STDTYPE_HPP
