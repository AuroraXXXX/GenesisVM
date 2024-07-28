//
// Created by aurora on 2022/12/11.
//

#include "VMError.hpp"
#include "plat/stream/FileCharOStream.hpp"
#include "plat/os/cpu.hpp"
#include "plat/utils/OrderAccess.hpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
volatile long VMError::_first_error_tid = -1;
const char *VMError::_filename;
int VMError::_lineno;
void *VMError::_pc;
const char *VMError::_message;
size_t VMError::_bytes;

int VMError::_current_step;
const char *VMError::_current_step_info;
VMErrorType VMError::_vm_error_type;
char VMError::_detail_msg[1024];


/**
 * 获取错误类型的字符串
 * @param error_type
 * @return
 */
static const char *get_vm_error_type_name(VMErrorType error_type) {
    switch (error_type) {
        case VMErrorType::INTERNAL_ERROR:
            return "internal";
        case VMErrorType::OOM_MALLOC_ERROR:
            return "malloc";
        case VMErrorType::OOM_MMAP_ERROR:
            return "mmap";
        case VMErrorType::OOM_MPROTECT_ERROR:
            return "mprotect";
    }
    return nullptr;
}

/**
 *
 * @param vm_error_type
 * @param filename
 * @param lineno
 * @param message
 * @param pc
 * @param detail_fmt
 * @param detail_args
 * @param bytes
 */
void VMError::report_and_die(VMErrorType vm_error_type,
                             const char *filename,
                             int lineno,
                             const char *message,
                             void *pc,
                             const char *detail_fmt,
                             va_list detail_args,
                             size_t bytes) {
    //内部静态缓冲区 不能申请线程的 防止错误
    static char buffer[OStreamDefaultBufSize];
    // 报告first_error时错误处理程序中发生了多少错误。
    static int recursive_error_count;
    //表明日志模块 是否初始化成功
    static bool log_done = false;

    //使用输出流 并使用静态缓冲区 进行缓冲
    auto log = FileCharOStream::error_stream();

    //log.set_internal_buf(buffer, sizeof(buffer));
    //log.print("123");
    long my_tid = os::current_thread_id();
    if (VMError::_first_error_tid == -1 &&
        OrderAccess::cas<long>(&VMError::_first_error_tid, -1, my_tid) == -1) {
        //说明 是第一个错误
        _vm_error_type = vm_error_type;
        _filename = filename;
        _lineno = lineno;
        _pc = pc;
        _message = message;
        _bytes = bytes;
        ::vsnprintf(_detail_msg, sizeof(_detail_msg), detail_fmt, detail_args);
    } else {
        //表明整个虚拟机中已经存在错误了
        //需要看看 错误是 来自于本线程 还是其他线程
        if (my_tid != VMError::_first_error_tid) {
            /**
             * 出现错误的线程和当前线程 不是来自于同一个线程
             * 那就说明 当前线程也有一个错误 且之前已经有一个错误了
             * 那么就需要 进行简单错误输出 说明当前线程也存在一个错误
             *
             * 错误输出结束了 那么当前的线程应该永久的休眠
             * 等待另外一个线程错误结束 由这个线程中止整个进程
             */
            char msgBuf[64];
            ::snprintf(msgBuf, sizeof(msgBuf), "[ID是%lu的线程也有一个错误]", my_tid);
            log->print_raw_cr(msgBuf);
            log->flush();
            //永久睡眠
            while (true) {
                ::sleep(100);
            }
        } else {
            /**
             * 说明之前的错误 和当前的错误 是来自于同一个线程
             */
            if (recursive_error_count++ > 30) {
                /**
                 * 报告first_error时错误处理程序中发生了超过30次的错误
                 * 输出 简单的错误信息
                 * 直接中止即可
                 */
                log->print_raw_cr("[太多的错误,调用abort,立刻中止.]");
                log->flush();
                ::abort();
            }
            /**
             * 提示用于在错误处理程序中
             * 出现了递归错误
             */
            ::snprintf(buffer,
                       sizeof(buffer),
                       "在报告(%s,id:%s)错误的过程中,又发生了错误."
                       "错误位置信息(%s:%d),pc=%p.",
                       message,
                       get_vm_error_type_name(_vm_error_type),
                       filename,
                       lineno,
                       pc);
            log->print_raw_cr(buffer);
        }
    }
    //下面进行实际的输出
    if (!log_done) {
        VMError::report(log, false);
        log->flush();
        /**
         * 数据输出结束 那么需要将相关的信息 恢复到原状
         */
        log_done = true;
        VMError::_current_step = 0;
        VMError::_current_step_info = "";
    }
    ::abort();
}

void VMError::report(CharOStream *stream, bool verbose) {
#define BEGIN if(VMError::_current_step == 0) { VMError::_current_step = __LINE__;
#define STEP(s) }if(VMError::_current_step < __LINE__) {\
                    VMError::_current_step = __LINE__;\
                    VMError::_current_step_info = s;
#define END }
    BEGIN

    STEP("打印错误类型中")
        switch (VMError::_vm_error_type) {
            case VMErrorType::OOM_MMAP_ERROR:
            case VMErrorType::OOM_MALLOC_ERROR:
            case VMErrorType::OOM_MPROTECT_ERROR:
                stream->print("# 原始内存(Native memory)申请 ");
                stream->print("(%s)获取"
                SIZE_FORMAT
                "字节时失败.",
                        get_vm_error_type_name(_vm_error_type));
                stream->cr();
            case VMErrorType::INTERNAL_ERROR:
            default:
                break;
        }
    STEP("打印错误信息中")
        stream->print("#  %s", (_message ? _message : "Error"));
        if (::strlen(_detail_msg) > 0) {
            stream->print(" : %s", _detail_msg);
        }
        stream->cr();
    STEP("打印进程和当前线程id")
    STEP("输出行号")
        stream->print_cr("位置在 %s:%d", _filename, _lineno);
    STEP("打印结束标记")
        stream->print_cr("错误打印结束");
    END
#undef BEGIN
#undef STEP
#undef END
}


