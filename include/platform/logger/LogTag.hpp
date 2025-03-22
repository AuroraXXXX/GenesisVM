//
// Created by aurora on 2025/2/10.
//

#ifndef PLATFORM_LOG_TAG_HPP
#define PLATFORM_LOG_TAG_HPP
#include "platform/typedef.hpp"
#define LOG_TAG_DECL(decl) \
decl(platform)             \
decl(safepoint)            \
decl(nonuserthread)               \
decl(metaspace)                           \
decl(robust)


enum class LogTag:uint16_t {
    no_tag = 0,
#define decl(tag) tag,
    LOG_TAG_DECL(decl)
#undef decl
};

#endif //PLATFORM_LOG_TAG_HPP
