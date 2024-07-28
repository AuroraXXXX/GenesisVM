//
// Created by aurora on 2023/10/8.
//

#include "ArenaChunkPool.hpp"
#include "ArenaChunk.hpp"
#include "GlobalLocker.hpp"

ArenaChunkPool *ArenaChunkPool::_tiny_pool = nullptr;
ArenaChunkPool *ArenaChunkPool::_small_pool = nullptr;
ArenaChunkPool *ArenaChunkPool::_medium_pool = nullptr;
ArenaChunkPool *ArenaChunkPool::_large_pool = nullptr;


ArenaChunk *ArenaChunkPool::alloc() {
    //进行内存的申请 但是需要考虑线程安全问题
    GlobalLocker gl;
    auto head = this->_list_head;
    if (this->_list_head) {
        this->_list_head = this->_list_head->next();
        head->set_next(nullptr);
    }
    return head;
}

void ArenaChunkPool::free(ArenaChunk *chunk) {
    //进行线程锁
    GlobalLocker gl;
    chunk->set_next(this->_list_head);
    this->_list_head = chunk;
}


void ArenaChunkPool::purge() {
    //所有线程的锁，保证线程的安全
    GlobalLocker gl;
    ArenaChunk *cur = this->_list_head;
    ArenaChunk *next;
    while (cur != nullptr) {
        next = cur->next();
        FREE_CHEAP_ARRAY(cur, MEMFLAG::Chunk);
        cur = next;
    }

}

void ArenaChunkPool::initialize() {
    ArenaChunkPool::_tiny_pool = new ArenaChunkPool();
    ArenaChunkPool::_small_pool = new ArenaChunkPool();
    ArenaChunkPool::_medium_pool = new ArenaChunkPool();
    ArenaChunkPool::_large_pool = new ArenaChunkPool();
}

void ArenaChunkPool::clean() {
    ArenaChunkPool::_tiny_pool->purge();
    ArenaChunkPool::_small_pool->purge();
    ArenaChunkPool::_medium_pool->purge();
    ArenaChunkPool::_large_pool->purge();
}

ArenaChunkPool *ArenaChunkPool::get_pool(size_t chunk_bytes) {
    switch (chunk_bytes) {
        case ArenaChunk::tiny_bytes:
            assert(ArenaChunkPool::_tiny_pool != nullptr, "必须被初始化");
            return ArenaChunkPool::_tiny_pool;
        case ArenaChunk::small_bytes:
            assert(ArenaChunkPool::_small_pool != nullptr, "必须被初始化");
            return ArenaChunkPool::_small_pool;
        case ArenaChunk::medium_bytes:
            assert(ArenaChunkPool::_medium_pool != nullptr, "必须被初始化");
            return ArenaChunkPool::_medium_pool;
        case ArenaChunk::large_bytes:
            assert(ArenaChunkPool::_large_pool != nullptr, "必须被初始化");
            return ArenaChunkPool::_large_pool;
        default:
            return nullptr;
    }
}


