//
// Created by aurora on 2023/9/11.
//
#include "plat/utils/robust.hpp"
#include "VMError.hpp"
#include <cstdarg>


extern void report_vm_error(
        const char *filename,
        int lineno,
        const char *msg,
        const char *detail_msg,
        ...) {
    va_list ap;
    va_start(ap, detail_msg);
    VMError::report_and_die(VMErrorType::INTERNAL_ERROR,
                            filename,
                            lineno,
                            msg,
                            nullptr,
                            detail_msg,
                            ap,
                            0);
    va_end(ap);
}

extern void report_vm_out_of_memory(
        const char *filename,
        int lineno,
        size_t required_bytes,
        VMErrorType type,
        const char *msg,
        ...
) {
    va_list ap;
    va_start(ap, msg);
    VMError::report_and_die(type,
                            filename,
                            lineno,
                            "",
                            nullptr,
                            msg,
                            ap,
                            0);
    va_end(ap);
}


