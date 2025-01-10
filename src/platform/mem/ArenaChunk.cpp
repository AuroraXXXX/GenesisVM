//
// Created by aurora on 2023/10/8.
//

#include "ArenaChunk.hpp"
#include "ArenaChunkPool.hpp"
#include "plat/utils/align.hpp"
void *ArenaChunk::operator new(size_t size, size_t len, bool exit_oom) {
    assert(size == sizeof(ArenaChunk), "weird request size");
    // Try to reuse a freed chunk from the pool
    const auto pool = ArenaChunkPool::get_pool(size);
    if (pool != nullptr) {
        const auto c = pool->alloc();
        if (c != nullptr) {
            assert(c->length() == len, "wrong length?");
            return c;
        }
    }
    // Either the pool was empty, or this is a non-standard length. Allocate a new Chunk from C-heap.
    size_t bytes = size + len;
    void *p = NEW_CHEAP_ARRAY(uint8_t, bytes, MEMFLAG::Chunk);
    if (p == nullptr && exit_oom) {
        vm_exit_out_of_memory(VMErrorType::OOM_MALLOC_ERROR, bytes, "Chunk::new");
    }
    // We rely on arena alignment <= malloc alignment.
    assert(is_aligned(p, BytesPerWord), "Chunk start address misaligned.");
    return p;
}

void ArenaChunk::operator delete(void *p) {
    auto segment = (ArenaChunk *) p;
    const auto pool = ArenaChunkPool::get_pool(segment->length());
    if(pool != nullptr){
        pool->free(segment);
    } else{
        FREE_CHEAP_ARRAY(segment,MEMFLAG::Chunk);
    }
}


bool ArenaChunk::contains(void *p) const {
    return (uintptr_t) p >= this->bottom_literal() && (uintptr_t) p <= this->end_literal();
}


ArenaChunk::ArenaChunk(size_t actual_bytes) :
        _next(nullptr),
        _end_literal((uintptr_t) this + actual_bytes) {

}


