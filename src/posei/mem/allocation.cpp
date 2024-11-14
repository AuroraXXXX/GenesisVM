//
// Created by aurora on 2023/9/10.
//

#include "plat/mem/allocation.hpp"
#include <malloc.h>
#include <cstdlib>
#include "plat/utils/robust.hpp"
#include "MemoryTracer.hpp"
#include "plat/mem/Arena.hpp"
#include "plat/thread/OSThread.hpp"
/**
 * -----------------------------
 * CHeap 内存申请和分配
 * -----------------------------
 */
extern const char *MEMFLAG_NAME(MEMFLAG flag) {
    switch (flag) {
#define MEMORY_FLAG_DECLARE_ENUM(type, human_readable) \
case MEMFLAG::type: return #type;
        MEMORY_FLAGS_DO(MEMORY_FLAG_DECLARE_ENUM)
#undef MEMORY_FLAG_DECLARE_ENUM
        default:
            return "unknown";
    }
}

extern void *CHEAP_ALLOC(MEMFLAG F,
                         size_t bytes,
                         bool exit_oom) {
    // 1 申请内存 然后进行内存的记录
    const auto p = ::malloc(bytes);
    if (p == nullptr) {
        if (!exit_oom) {
            return p;
        }
        vm_exit_out_of_memory(VMErrorType::OOM_MALLOC_ERROR,
                              bytes,
                              "申请%s类型的" SIZE_FORMAT "字节内存失败",
                              MEMFLAG_NAME(F),
                              bytes);
    }

    auto size = ::malloc_usable_size(p);
    MemoryTracer::record( F,
                         MemoryTracer::OperationType::native_alloc,
                         p,
                         size,
                         CALLER_STACK);

    return p;
}

extern void *CHEAP_ALLOC_ALIGN(
        MEMFLAG F,
        size_t bytes,
        size_t align,
        bool exit_oom) {
    void *value = nullptr;
    //要求内存对齐到指定长度
    auto res = ::posix_memalign(&value, bytes, align);
    if (res != 0) {
        //说明失败了 如果是要求退出虚拟机那么就进行退出
        if (!exit_oom) {
            return nullptr;
        }
        vm_exit_out_of_memory(VMErrorType::OOM_MALLOC_ERROR,
                              bytes,
                              "申请%s类型的" SIZE_FORMAT "字节,并要求对齐到的"
                                      SIZE_FORMAT "内存失败",
                              MEMFLAG_NAME(F),
                              bytes,
                              align);
    }
    //说明申请成功了
    MemoryTracer::record( F,
                         MemoryTracer::OperationType::native_alloc,
                         value,
                         bytes,
                         CALLER_STACK);
    return value;
}

extern void CHEAP_FREE(MEMFLAG F, void *p) {
    auto size = ::malloc_usable_size(p);
    MemoryTracer::record( F,
                         MemoryTracer::OperationType::native_free,
                         p,
                         size,
                         CALLER_STACK);
    ::free(p);
}

/**
 * -----------------------------
 * Arena 内存申请和分配
 * -----------------------------
 */
extern void *ARENA_ALLOC(
        Arena *arena,
        size_t bytes,
        bool exit_oom){
    return arena->alloc(bytes,exit_oom);
}

extern void ARENA_FREE(
        Arena *arena,
        void *ptr,
        size_t bytes){
    arena->free(ptr,bytes);
}

/**
 * -----------------------------
 * 线程栈资源申请和分配
 * -----------------------------
 */
extern void *RESOURCE_ARENA_ALLOC(
        size_t bytes,
        bool exit_oom){
    const auto arena =    OSThread::current()->resource_arena();
    return ARENA_ALLOC(arena,bytes,exit_oom);
}

extern void RESOURCE_ARENA_FREE(void *ptr, size_t bytes){
    const auto arena =    OSThread::current()->resource_arena();
    ARENA_FREE(arena,ptr,bytes);
}

