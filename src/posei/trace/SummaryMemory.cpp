//
// Created by aurora on 2024/6/25.
//
#include "SummaryMemory.hpp"
#include "plat/utils/OrderAccess.hpp"
#include <malloc.h>
#include "plat/utils/robust.hpp"
#include "plat/stream/OStream.hpp"
#include "plat/mem/allocation.hpp"

SummaryMemory::Unit SummaryMemory::_unit[];



void SummaryMemory::summary(MEMFLAG F,
                            MemoryTracer::OperationType type,
                            size_t bytes) {
    const auto unit = SummaryMemory::_unit + (int32_t)F;
    switch (type) {
        case MemoryTracer::OperationType::reserve:
            OrderAccess::fetch_and_add(&unit->_virtual_reserved, bytes);
            break;
        case MemoryTracer::OperationType::commit:
            OrderAccess::fetch_and_add(&unit->_virtual_committed, bytes);
            break;
        case MemoryTracer::OperationType::uncommit:
            OrderAccess::fetch_and_sub(&unit->_virtual_committed, bytes);
            break;
        case MemoryTracer::OperationType::release:
            OrderAccess::fetch_and_sub(&unit->_virtual_reserved, bytes);
            break;

        case MemoryTracer::OperationType::native_alloc:
            OrderAccess::fetch_and_add(&unit->_native_alloc, bytes);
            OrderAccess::fetch_and_add<size_t>(&unit->_native_count, 1);
            break;

        case MemoryTracer::OperationType::native_free:
            OrderAccess::fetch_and_sub(&unit->_native_alloc, bytes);
            OrderAccess::fetch_and_sub<size_t>(&unit->_native_count, 1);
            break;

        case MemoryTracer::OperationType::arena_alloc:
            OrderAccess::fetch_and_add(&unit->_arena_alloc, bytes);
            OrderAccess::fetch_and_add<size_t>(&unit->_arena_count, 1);
            break;

        case MemoryTracer::OperationType::arena_free:
            OrderAccess::fetch_and_add(&unit->_arena_alloc, bytes);
            OrderAccess::fetch_and_add<size_t>(&unit->_arena_count, 1);
            break;
        case MemoryTracer::OperationType::max:
            should_not_reach_here();
            break;
    }
}

void SummaryMemory::output(OStream *stream) {
    if (stream == nullptr) {
        return;
    }
    stream->lock();
    for (uint8_t i = 0; i < SummaryMemory::max_tag; ++i) {
        const auto unit = SummaryMemory::_unit + i;
        stream->write_uint64(unit->_virtual_reserved);
        stream->write_uint64(unit->_virtual_committed);
        stream->write_uint64(unit->_arena_alloc);
        stream->write_uint64(unit->_arena_count);
        stream->write_uint64(unit->_native_alloc);
        stream->write_uint64(unit->_native_count);
    }
    stream->unlock();
}


SummaryMemory::Unit::Unit() noexcept {
    this->_virtual_reserved = 0;
    this->_virtual_committed = 0;
    this->_arena_alloc = 0;
    this->_arena_count = 0;
    this->_native_alloc = 0;
    this->_native_count = 0;
}
