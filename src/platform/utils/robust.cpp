//
// Created by aurora on 2024/11/19.
//
#include "platform/utils/robust.hpp"
#include <cstdlib>
void report_vm_error(
        const char *filename,
        int lineno,
        const char *prefix,
        const char *detail_msg,
        ...){

}


 void report_vm_out_of_memory(
        const char *filename,
        int lineno,
        size_t required_bytes,
        VMErrorType type,
        const char *msg,
        ...
){

}