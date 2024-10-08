//
// Created by aurora on 2024/1/7.
//

#ifndef LOGGING_CONSTANTS_HPP
#define LOGGING_CONSTANTS_HPP

#include "stdtype.hpp"
#include "plat/constants.hpp"
#include "global/LogTag.hpp"

/**
 * LogNoTag中LogTag中特殊的表示，表示没有LogTag
 */
#define PREFIX_LOG_TAG(T) LogTag::T


constexpr inline uint16_t LogTagSetMax = 5;
constexpr inline size_t LOG_MAX_FOLLOWER_SIZE = 256;


#define LOG_LEVEL_LIST(def) \
def(trace)                  \
def(debug)                  \
def(info)                   \
def(warn)                   \
def(error)                  \
def(off)

enum class LogLevel : uint8_t {
#define LOG_LEVEL(level) level,
    LOG_LEVEL_LIST(LOG_LEVEL)
#undef LOG_LEVEL
    count,
    default_console_level = trace //一开始输出
};


#endif //LOGGING_CONSTANTS_HPP
