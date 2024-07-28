//
// Created by aurora on 2024/6/25.
//
#include "plat/globals.hpp"

namespace global {
#define DECLARE_PRODUCT_FLAG(type, name, value, ...) type name = value;

#ifdef DEVELOP
#define DECLARE_DEVELOP_FLAG(type,name,value,...)
#else
#define DECLARE_DEVELOP_FLAG(type, name, value, ...) type name = value;
#endif
    PLATFORM_FLAGS(DECLARE_PRODUCT_FLAG, DECLARE_DEVELOP_FLAG, IGNORE_RANGE)

}