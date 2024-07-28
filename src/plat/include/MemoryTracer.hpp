//
// Created by aurora on 2024/6/25.
//

#ifndef PLAT_MEMORY_TRACER_HPP
#define PLAT_MEMORY_TRACER_HPP

#include "stdtype.hpp"
#include "plat/utils/NativeCallStack.hpp"
#include "plat/mem/allocation.hpp"

class MemoryTracer {
public:
    enum class OperationType : uint8_t {
        reserve = 0,
        commit,
        uncommit,
        release,
        native_alloc,
        native_free,
        arena_alloc,
        arena_free,
        max //不可以使用
    };
    enum class NMT_Level : uint8_t {
        unknown,
        off,
        summary,
        detail
    };
private:
    static NMT_Level _nmt_level;
public:
    /**
     * 记录内存记录
     * @param F 内存类型
     * @param type 操作类型
     * @param addr 内存地址
     * @param bytes 内存大小
     * @param call_stack 调用堆栈
     */
    static void record(MEMFLAG F,
                       MemoryTracer::OperationType type,
                       void *addr,
                       size_t bytes,
                       const NativeCallStack &call_stack);

    static void initialize();

    static void flush();
};

#endif //PLAT_MEMORY_TRACER_HPP
