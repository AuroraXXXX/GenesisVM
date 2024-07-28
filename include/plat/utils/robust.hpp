//
// Created by aurora on 2023/9/9.
//

#ifndef PLATFORM_UTILS_ROBUST_HPP
#define PLATFORM_UTILS_ROBUST_HPP

#include "stdtype.hpp"
#include "plat/macro.hpp"

enum class VMErrorType {
    INTERNAL_ERROR, //内部的错误
    OOM_MALLOC_ERROR,//malloc申请内存不足 与内存相关的错误
    OOM_MMAP_ERROR,  //mmap申请不足错误 与内存相关的错误
    OOM_MPROTECT_ERROR
};


/**
 * 用于提升系统健壮性的代码
 * 注意此处存在一个接口
 * 用于将输出转换到日志框架上
 */
class CharOStream;

/**
 * 报告虚拟机发生的错误
 * @param filename 发生错误的文件名
 * @param lineno 发生错误的行号
 * @param prefix 虚拟机发生错误的基本信息
 * @param detail_msg 错误的详细信息 支持字符串模板
 * @param ... 模板的参数
 */
extern void report_vm_error(
        const char *filename,
        int lineno,
        const char *prefix,
        const char *detail_msg = nullptr,
        ...);

/**
 * 报告虚拟机中溢出内存的错误
 * @param filename
 * @param lineno
 * @param required_bytes
 * @param type
 * @param msg
 * @param ...
 */
extern void report_vm_out_of_memory(
        const char *filename,
        int lineno,
        size_t required_bytes,
        VMErrorType type,
        const char *msg,
        ...
);



/**
 * assert 静态断言 必须开启ASSERT
 * @param p 校验的表达式
 * @param msg 验证错误后的输出的详细消息
 * @param ... 详细消息的参数
 */

#define assert(p, msg, args...)  DEBUG_MODE_ONLY(                    \
do{                                                                 \
    if(!(p)){                                                       \
        report_vm_error(__FILE__,                         \
                        __LINE__,                                   \
                        "断言(" #p ")失败",                          \
                        msg,                                        \
                        ##args);                                    \
    }                                                               \
}while(0) )

/**
 * 同assert一样 但是不开启ASSERT也可以使用
 * @param p 校验的表达式
 * @param msg 验证错误后的输出的详细消息
 * @param args... 详细消息的参数
 */
#define guarantee(p, msg, args...)                                   \
do{                                                                 \
    if(!(p)){                                                       \
        report_vm_error(__FILE__,                                   \
                        __LINE__,                                   \
                        "担保(" #p ")失败",                          \
                        msg,                                        \
                        ##args);                                    \
    }                                                               \
}while(0)

#define should_not_reach_here()                                        \
        report_vm_error(__FILE__,                                      \
                        __LINE__,                                      \
                        "不应该执行到此处")

#define unimplemented(info)                                         \
        report_vm_error(__FILE__,                                   \
                        __LINE__,                                   \
                        "功能未实现" info)
/**
 * 虚拟机启动过程中出现错误
 */
#define vm_exit_during_initialization(message, args...)              \
        report_vm_error(__FILE__,                                   \
                        __LINE__,                                   \
                        "虚拟机启动过程中错误退出:",                    \
                        message,                                    \
                        ##args)
#define vm_exit_out_of_memory(error_type, required_bytes, msg, args...)      \
        do{                                                         \
             report_vm_out_of_memory(__FILE__,                      \
                                     __LINE__,                      \
                                    required_bytes,                 \
                                    error_type,                     \
                                    msg,                           \
                                    ##args);                           \
        }while(0)


#endif //PLATFORM_UTILS_ROBUST_HPP
