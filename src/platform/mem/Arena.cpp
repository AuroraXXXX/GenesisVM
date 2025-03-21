//
// Created by aurora on 2022/4/12.
//

#include "platform/mem/Arena.hpp"
#include "platform/constants.hpp"
#include "ArenaChunk.hpp"
#include "MemoryTracer.hpp"
#include "ArenaChunkPool.hpp"
#include "platform/utils/align.hpp"
void Arena::new_chunk(size_t chunk_bytes,
                      bool exit_oom) {
    assert_is_aligned(chunk_bytes, BytesPerWord);
   const  auto chunk = new(chunk_bytes, exit_oom)
            ArenaChunk(chunk_bytes);
    MemoryTracer::record( this->flag(),
                         MemoryTracer::OperationType::arena_alloc,
                         chunk,
                         chunk_bytes,
                         caller_address);
    this->_total_bytes += chunk_bytes;
    this->_top_literal = chunk->bottom_literal();
    this->_end_literal = chunk->end_literal();
    //将元素压入到栈中
    this->_list.push(chunk);
}

size_t Arena:: chop_list(ArenaChunk* old_stack_top)  {
    size_t total_free_bytes = 0;
    while (true) {
        if(this->_list.peek() == old_stack_top){
            //说明栈顶元素就是原本的栈顶元素 那么就说明我们要进行退出了，不要进行删除操作了
            break;
        }
        if(this->_list.is_empty()){
            //说明已经没有元素了 那么就退出吧
            break;
        }
        //获取栈顶元素
        auto  chunk =  this->_list.pop();
        //计算释放的内存大小
        total_free_bytes += chunk->length();
        //记录内存释放
        MemoryTracer::record( this->flag(),
                              MemoryTracer::OperationType::arena_free,
                              chunk,
                              chunk->length(),
                              caller_address);
        //释放内存
        delete chunk;
    }
    return total_free_bytes;
}


Arena::Arena(MEMFLAG flag, size_t init_bytes) :
        _flag(flag),
        _top_literal(0),
        _end_literal(0),
        _list(),
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
    //释放所有内存
    this->_total_bytes = this->chop_list(nullptr);
    assert(this->_total_bytes == 0, "must be");
    /**
     * 将所持有的数据信息全部清空
     */
    this->_list.clear();
    this->_top_literal = 0;
    this->_end_literal = 0;
}

Arena::Arena(MEMFLAG F) :
        Arena(F, ArenaChunk::small_bytes) {
}

void Arena::iter_chunk(Arena::ChunkClosure *closure) {
    bool is_first = true;
    const auto iter_func = [&](ArenaChunk* chunk){
        if (is_first){
            //对于第1块 需要特殊的处理 因为的第一块并不是完全使用的
            closure->do_chunk((void *) chunk->bottom_literal(),
                              (void *) this->_top_literal);
            //说明下面的不是第一个
            is_first = false;
        } else{
            /**
             * 之后的全部视为使用完毕，进行遍历
             */
            closure->do_chunk((void *) chunk->bottom_literal(),
                              (void *) chunk->end_literal());
        }
        return true;
    };
    this->_list.iterate(iter_func);

}

void Arena::clean_pool() {
    ArenaChunkPool::clean();
}



Arena::SavedData::SavedData(Arena *arena) :
        _current_top(arena->_list.peek()),
        _total_bytes(arena->_total_bytes),
        _end_literal(arena->_end_literal),
        _top_literal(arena->_top_literal) {
    assert(arena != nullptr, "must be");
}

void Arena::SavedData::rollback_to(Arena *arena) {
    assert(arena != nullptr, "must be");
    arena->_top_literal = this->_top_literal;
    arena->_end_literal = this->_end_literal;
    auto need_free_bytes = arena->_total_bytes - this->_total_bytes;
    arena->_total_bytes = this->_total_bytes;
    auto  total_bytes= arena->chop_list(this->_current_top);
    assert(need_free_bytes == total_bytes,"check");
}

