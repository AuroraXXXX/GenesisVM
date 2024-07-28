//
// Created by aurora on 2024/6/24.
//

#include "DetailLogMemory.hpp"
#include "plat/os/cpu.hpp"
#include "plat/stream/FileCharOStream.hpp"
#include "plat/utils/ByteOrder.hpp"
#include "plat/utils/robust.hpp"
#include "plat/globals.hpp"

std::atomic<uint16_t> DetailLogMemory::_next_order_id = 0;
OStream* DetailLogMemory::_stream = nullptr;
void DetailLogMemory::global_initialize() {
    static FileCharOStream stream(global::NMTFilePath);
    if (stream.is_open()) {
        DetailLogMemory::_stream = &stream;
    }
    guarantee(stream.is_open(), "native memory tracer file initialize failed");

}

void DetailLogMemory::detail_log(MEMFLAG F,
                                 MemoryTracer::OperationType type,
                                 void *addr,
                                 size_t bytes,
                                 const NativeCallStack &call_stack) {
    Unit detail;
    detail._memory_tag = (int32_t)F;
    detail._operation_type = (uint8_t) type;
    const auto order_id = DetailLogMemory::_next_order_id.fetch_add(1);
    detail._order_id = ByteOrder::network(order_id);
    detail._thread_id = ByteOrder::network(os::current_thread_id());
    detail._addr = ByteOrder::network((uintptr_t)addr);
    detail._bytes = ByteOrder::network(bytes);
    if(!call_stack.is_empty()){
        void** bottom =  (void **)call_stack.stack();
        for (int32_t i = 0; i < NativeCallStack::MAX_DEPTH; ++i) {
           detail._caller[i] = ByteOrder::network((uintptr_t)bottom[i]);
        }
    }
    const auto stream = DetailLogMemory::_stream;
    stream->lock();
    stream->write_bytes(&detail, sizeof(Unit));
    stream->unlock();
}

void DetailLogMemory::flush() {
    auto stream = DetailLogMemory::_stream;
    if(stream != nullptr){
        stream->flush();
    }
}


