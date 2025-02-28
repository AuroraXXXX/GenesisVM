//
// Created by aurora on 2025/2/11.
//
#include "platform/logger/LogTag.hpp"


 const char * LOG_TAG_NAMES[]= {
        "",
#define decl(tag) #tag,
        LOG_TAG_DECL(decl)
#undef decl
};
