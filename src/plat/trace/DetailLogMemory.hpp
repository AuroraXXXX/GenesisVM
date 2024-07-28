//
// Created by aurora on 2024/6/24.
//

#ifndef PLAT_TRACE_DETAIL_LOG_MEMORY_HPP
#define PLAT_TRACE_DETAIL_LOG_MEMORY_HPP

#include "stdtype.hpp"
#include "atomic"
#include "plat/utils/NativeCallStack.hpp"
#include "MemoryTracer.hpp"
#include "plat/mem/allocation.hpp"
class OStream;

class DetailLogMemory {
private:
    struct Unit {
        uint8_t _memory_tag;
        uint8_t _operation_type;
        uint16_t _order_id;
        uint32_t _thread_id;
        uintptr_t _addr;
        size_t _bytes;
        uintptr_t _caller[NativeCallStack::MAX_DEPTH];
    };
    static OStream *_stream;
    static std::atomic<uint16_t> _next_order_id;
public:
    static inline auto stream() {
        return _stream;
    };

    /**
     * Q
     */
    static void global_initialize();

    /**
     * 进行详细的记录
     * @param F 内存类型
     * @param type 操作类型
     * @param bytes 操作的字节数
     * @param call_stack
     */
    static void detail_log(MEMFLAG F,
                           MemoryTracer::OperationType type,
                           void *addr,
                           size_t bytes,
                           const NativeCallStack &call_stack);

    static void flush();
};


#endif //PLAT_TRACE_DETAIL_LOG_MEMORY_HPP
