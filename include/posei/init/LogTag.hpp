//
// Created by aurora on 2024/8/6.
//

#ifndef GENESIS_VM_GLOBAL_LOG_TAG_HPP
#define GENESIS_VM_GLOBAL_LOG_TAG_HPP
#include "stdtype.hpp"
#define LOG_TAG_LIST(def)   \
    def(safepoint)          \
    def(vmthread)           \
    def(metaspace)          \
    def(gc)

enum class LogTag:uint16_t {
    no_tag,
#define LOG_TAG_DEF(name) name,
    LOG_TAG_LIST(LOG_TAG_DEF)
#undef LOG_TAG_DEF
};

#endif //GENESIS_VM_GLOBAL_LOG_TAG_HPP
