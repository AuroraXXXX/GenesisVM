//
// Created by aurora on 2024/6/25.
//

#ifndef PLAT_GLOBALS_HPP
#define PLAT_GLOBALS_HPP

#include "stdtype.hpp"

namespace global {
/**
 * 定义区间的
 */
#define IGNORE_RANGE(a, b)
#define DECLARE_PRODUCT_FLAG(type, name, value, ...) extern "C" type name;

#ifdef DEVELOP
#define DECLARE_DEVELOP_FLAG(type,name,value,...)
#else
#define DECLARE_DEVELOP_FLAG(type, name, value, ...) extern "C" type name;
#endif

#define PLATFORM_FLAGS(product, develop, range) \
    product(const char* ,ErrorFilePath,"/tmp/demo.log","致命错误输出到文件") \
    product(const char *,NMTLevel,"detail","内存追踪模式。off不开启,summary记录基本信息,detail记录详细信息")                              \
    product(const char *,NMTFilePath,"/tmp/os_memory.trace","本地内存追踪文件路径")\
    product(bool,OutputToStderr,true,"将输出到标准输出流")                               \
    product(bool,UseThreadPriority,true,"是否开启ThreadPriority")                       \
    product(int16_t ,ThreadPriority1,-1,"1对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority2,-1,"2对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority3,-1,"3对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority4,-1,"4对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority5,-1,"5对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority6,-1,"6对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority7,-1,"7对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority8,-1,"8对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority9,-1,"9对应到底层的线程优先级,-1表示默认")               \
    product(int16_t ,ThreadPriority10,-1,"10对应到底层的线程优先级,-1表示默认")





    PLATFORM_FLAGS(DECLARE_PRODUCT_FLAG, DECLARE_DEVELOP_FLAG, IGNORE_RANGE)
}
#endif //PLAT_GLOBALS_HPP
