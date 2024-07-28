//
// Created by aurora on 2022/12/11.
//

#ifndef PLAT_UTILS_VM_ERROR_HPP
#define PLAT_UTILS_VM_ERROR_HPP

#include "plat/utils/robust.hpp"
#include "plat/mem/AllStatic.hpp"


#include <cstdarg>


/**
 * 表示虚拟机中的错误
 *
 * 在发生内存溢出错误时，此时已经无法申请内存，
 * 所以全部是静态的，这样才可以进行错误的输出
 *
 */
class VMError : public AllStatic {
private:
    /**
     * 表示 第一个错误ID的线程
     * 如果 是-1 表示并没有错误消息输出
     */
    static volatile long _first_error_tid;
    /**
     * 发生错误的 文件名和行号
     */
    static const char *_filename;
    static int _lineno;
    /**
     * 发生错误的地址
     */
    static void *_pc;
    static const char *_message;
    /**
     * 记录因为内存溢出时候 内存大小
     */
    static size_t _bytes;
    /**
     * 用于记录错误处理时候
     * 可能出现的错误
     */
    static int _current_step;
    static const char *_current_step_info;
    static VMErrorType _vm_error_type;
    static char _detail_msg[1024];

    /**
     * 进行错误报告的主要函数
     * 只能在一个线程调用这个函数 所以无需担心线程的安全
     * 但是错误处理函数可能会因为内部问题错误而崩溃死亡
     * 例如堆栈严重损坏的时候
     * 因此我们需要处理递归错误
     *
     * @param stream 输出流
     * @param verbose 是否输出详细信息
     */
    static void report(CharOStream *stream, bool verbose);

public:
    /**
     * 错误的输出函数
     * 内部的实现：
     *
     * @param vm_error_type 错误的类型
     * @param filename 错误所在源文件的名称
     * @param lineno 错误所在源文件中的行号
     * @param message 错误的主要的消息
     * @param pc 错误的所在的虚拟地址
     * @param detail_fmt 错误详细信息的格式化字符串
     * @param detail_args 错误详细信息格式化所需参数
     * @param bytes 当是内存溢出错误时，申请的内存数，如果不是这个错误，这个参数无效，可随便填写
     */
    static void report_and_die(VMErrorType vm_error_type,
                               const char *filename,
                               int lineno,
                               const char *message,
                               void *pc,
                               const char *detail_fmt,
                               va_list detail_args,
                               size_t bytes);
};


#endif //PLAT_UTILS_VM_ERROR_HPP
