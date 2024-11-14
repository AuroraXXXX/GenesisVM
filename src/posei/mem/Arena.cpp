//
// Created by aurora on 2022/4/12.
//

#include "plat/mem/Arena.hpp"
#include "plat/constants.hpp"
#include "ArenaChunk.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "plat/utils/robust.hpp"
#include "plat/utils/align.hpp"
void Arena::new_chunk(size_t chunk_bytes,
                      bool exit_oom) {
    assert_is_aligned(chunk_bytes, BytesPerWord);
    auto chunk = new(chunk_bytes, exit_oom)
            ArenaChunk(chunk_bytes);
    if (chunk == nullptr) {
        //申请失败 返回
        return;
    }
    MemoryTracer::record( this->flag(),
                         MemoryTracer::OperationType::arena_alloc,
                         chunk,
                         chunk_bytes,
                         CALLER_STACK);
    this->_total_bytes += chunk_bytes;
    this->_top_literal = chunk->bottom_literal();
    this->_end_literal = chunk->end_literal();
    if (this->_head == nullptr) {
        assert(this->_tail == nullptr, "check");
        this->_tail = this->_head = chunk;
    } else {
        //插入到尾部
        chunk->set_next(this->_tail);
        this->_tail = chunk;
    }

}

size_t Arena::chop_list(ArenaChunk *chunk) const {
    ArenaChunk *cur = chunk;
    ArenaChunk *next;
    size_t total_free_bytes = 0;
    while (cur != nullptr) {
        MemoryTracer::record( this->flag(),
                             MemoryTracer::OperationType::arena_free,
                             cur,
                             cur->length(),
                             CALLER_STACK);
        next = cur->next();
        total_free_bytes += cur->length();
        delete cur;
        //删除后原本的cur就不可以访问了
        cur = next;
    }
    return total_free_bytes;
}


Arena::Arena(MEMFLAG flag, size_t init_bytes) :
        _flag(flag),
        _top_literal(0),
        _end_literal(0),
        _head(nullptr),
        _tail(nullptr),
        _total_bytes(0) {
    //对可使用的长度 进行对齐 应该机器最大的对宽度对齐
    init_bytes = align_up(init_bytes, BytesPerWord);
    this->new_chunk(init_bytes, true);
}

void *Arena::alloc(size_t request,bool exit_oom) {
    //进行内存的对齐 按照计算机的字宽对齐
    request = align_up(request, BytesPerWord);

    if (this->check_overflow(request, exit_oom)) {
        return nullptr;
    }

    if (this->_end_literal - this->_top_literal < request) {
        //说明当前申请不下 那么默认当前块使用完毕 重新申请 当前块就浪费了
        auto new_chunk_bytes = MAX2<size_t>(request, ArenaChunk::large_bytes);
        this->new_chunk(new_chunk_bytes, exit_oom);
    }
    /**
     * 当前的chunk可以申请的下
     * 使用类似于指针碰撞的方法进行了申请
     */
    auto old = this->_top_literal;
    this->_top_literal += request;
    return (void *) old;
}

bool Arena::check_overflow(
        size_t request,
        bool exit_oom) const {
    /**
     * max_uintx 的字节长度与指针相同 所以完全可以用来表示指针的最大表示数值
     * 减去需要申请的大小
     * 那么就是这个数值应该是内存的最低的开始范围
     *
     * 如果这个开始范围 还小于现在的下次可以申请的起始 那么肯定是溢出了
     */
    if (SIZE_MAX - request < this->_top_literal) {
        if (exit_oom) {
            vm_exit_out_of_memory(VMErrorType::OOM_MALLOC_ERROR, request, "Arena overflow");
        }else{
            return true;
        }
    }
    return false;
}


bool Arena::free(void *ptr, size_t request) {
    if (ptr == nullptr)
        return true;
    request = align_up(request, BytesPerWord);
    if ((uintptr_t) ptr + request == this->_top_literal) {
        this->_top_literal = (uintptr_t) ptr;
        return true;
    } else {
        return false;
    }
}

Arena::~Arena() {
    this->_total_bytes = this->chop_list(this->_head);
    assert(this->_total_bytes == 0, "must be");
    /**
     * 将所持有的数据信息全部清空
     */
    this->_head = nullptr;
    this->_tail = nullptr;
    this->_top_literal = 0;
    this->_end_literal = 0;
}

Arena::Arena(MEMFLAG F) :
        Arena(F, ArenaChunk::small_bytes) {
}

void Arena::iter_chunk(Arena::ChunkClosure *closure) {
    auto cur = this->_head;
    if (cur == nullptr) {
        return;
    }
    //对于第1块 需要特殊的处理
    closure->do_chunk((void *) cur->bottom_literal(),
                      (void *) this->_top_literal);
    cur = cur->next();
    //对于之后 内存块处理
    while (cur != nullptr) {
        closure->do_chunk((void *) cur->bottom_literal(),
                          (void *) cur->end_literal());
    }
}

void Arena::clean_pool() {
    ArenaChunkPool::clean();
}



Arena::SavedData::SavedData(Arena *arena) :
        _tail(arena->_tail),
        _total_bytes(arena->_total_bytes),
        _end_literal(arena->_end_literal),
        _top_literal(arena->_top_literal) {
    assert(arena != nullptr, "must be");
}

void Arena::SavedData::rollback_to(Arena *arena) {
    assert(arena != nullptr, "must be");
    arena->_top_literal = this->_top_literal;
    arena->_end_literal = this->_end_literal;
    arena->_tail = this->_tail;
    arena->_total_bytes = this->_total_bytes;
    this->_tail->set_next(nullptr);
    auto delete_node = this->_tail->next();
    arena->chop_list(delete_node);
}

