//
// Created by aurora on 2024/6/25.
//
#include "global/flag.hpp"

namespace global {
#ifdef DECLARE_PRODUCT_FLAG
    #undef DECLARE_PRODUCT_FLAG
#endif
#define DECLARE_PRODUCT_FLAG(type, name, value, ...) type name = value;

#ifdef DECLARE_DEVELOP_FLAG
    #undef DECLARE_DEVELOP_FLAG
#endif

#ifdef DEVELOP
#define DECLARE_DEVELOP_FLAG(type,name,value,...)
#else
#define DECLARE_DEVELOP_FLAG(type, name, value, ...) type name = value;
#endif

    PLATFORM_FLAGS(DECLARE_PRODUCT_FLAG, DECLARE_DEVELOP_FLAG, IGNORE_RANGE)

    METASPACE_FLAGS(DECLARE_PRODUCT_FLAG, DECLARE_DEVELOP_FLAG, IGNORE_RANGE)
}