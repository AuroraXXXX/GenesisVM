//
// Created by aurora on 2024/7/16.
//

#ifndef KERNEL_UTILS_LOG_HPP
#define KERNEL_UTILS_LOG_HPP
#include "plat/logger/log.hpp"
#define LOG_TAG_LIST(def) \
    def(safepoint)        \
    def(vmthread)

enum {
#define LOG_TAG_DEF(name) PREFIX_LOG_TAG(name),
    LOG_TAG_LIST(LOG_TAG_DEF)
#undef LOG_TAG_DEF
};

#endif //KERNEL_UTILS_LOG_HPP
