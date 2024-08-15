//
// Created by aurora on 2024/6/25.
//

#ifndef GENESIS_VM_GLOBAL_FLAG_HPP
#define GENESIS_VM_GLOBAL_FLAG_HPP

#include "stdtype.hpp"
#include "plat/constants.hpp"
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
    product(int16_t ,ThreadPriority10,-1,"10对应到底层的线程优先级,-1表示默认")             \
    product(bool,AlwaysPreTouch,true,"预先实际获取内存")                                 \

#define METASPACE_FLAGS(product,develop,range) \
    product(size_t,MaxMetaspaceSize,SIZE_MAX,"元空间最大的大小")             \
    product(size_t,MetaspaceSize,21*M,"初始阈值(以字节为单位),以及最小大小")                  \
    product(size_t,MinMetaspaceFreeRatio,40,"最小空闲比例")                               \
    range(0,100)                                    \
    product(size_t,MaxMetaspaceFreeRatio,70,"最大空闲比例")                                   \
    range(0,100)                                    \
    product(size_t,MaxMetaspaceExpansion, 4 * M  ,"GC的情况下,Metaspace的最大扩展(以字节为单位)") \
    product(size_t,MinMetaspaceExpansion,256 * K ,"Metaspace的最小扩展(以字节为单位)")                  \



    PLATFORM_FLAGS(DECLARE_PRODUCT_FLAG, DECLARE_DEVELOP_FLAG, IGNORE_RANGE)

    METASPACE_FLAGS(DECLARE_PRODUCT_FLAG, DECLARE_DEVELOP_FLAG, IGNORE_RANGE)
}
#endif //GENESIS_VM_GLOBAL_FLAG_HPP
