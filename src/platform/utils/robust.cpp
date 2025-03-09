//
// Created by aurora on 2024/11/19.
//
#include "platform/utils/robust.hpp"
#include <cstdarg>
#include "platform/log.hpp"
#include "platform/concurrent/OSThread.hpp"
#include "platform/concurrent/Mutex.hpp"
#include "platform/concurrent/safepoint.hpp"
static const char * VMErrorTypeName[] =  {
    "INTERNAL_ERROR", //内部的错误
    "OOM_MALLOC_ERROR",//malloc申请内存不足 与内存相关的错误
    "OOM_MMAP_ERROR",  //mmap申请不足错误 与内存相关的错误
    "OOM_MPROTECT_ERROR"
};
void report_vm_error(
        const char *filename,
        int lineno,
        const char *prefix,
        const char *detail_msg,
        ...){
    ResourceArenaMark mark;
    log_stream(error,robust);
    log.print("(%s:%d)  %s",filename,lineno,prefix);
    if(detail_msg != nullptr){
        va_list args;
        va_start(args,detail_msg);
        log.print_va_list_cr(detail_msg,args);
        va_end(args);
    }
}


 void report_vm_out_of_memory(
        const char *filename,
        int lineno,
        size_t required_bytes,
        VMErrorType type,
        const char *msg,
        ...
){
     ResourceArenaMark mark;
     log_stream(error,robust);
     log.print("(%s:%d) request " SIZE_FORMAT " bytes error( %s ).",filename,lineno,required_bytes,VMErrorTypeName[(int32_t)type]);
     if(msg != nullptr){
         va_list args;
         va_start(args,msg);
         log.print_va_list_cr(msg,args);
         va_end(args);
     }
}

#ifdef DEBUG_MODE_ONLY

extern void assert_lock_strong(Mutex *lock) {
    assert(lock != nullptr, "需要一个不为nullptr的锁");
    assert(lock->owned_by_self(), "必须拥有锁:%s", lock->name());
}
extern void assert_locked_or_safepoint(Mutex* lock){
    assert(lock != nullptr, "需要一个不为nullptr的锁");
    if(lock->owned_by_self())return;
    if(Safepoint::is_at_safepoint()) return;
    assert(false,"必须拥有锁:%s,或者在安全点", lock->name());
}
#endif
