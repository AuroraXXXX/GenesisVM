//
// Created by aurora on 2023/10/8.
//

#ifndef PLAT_ARENA_CHUNK_HPP
#define PLAT_ARENA_CHUNK_HPP

#include "plat/constants.hpp"
#include "plat/mem/allocation.hpp"

/**
 * 快速内存的内存块
 * 底层使用内存池进行优化管理,避免频繁的系统调用
 */
class ArenaChunk {
private:
    /**
     * 用于指向下一个内存块
     */
    ArenaChunk *_next;
    const uintptr_t _end_literal;

public:

    enum : uint32_t {
        slack_bytes = 40,//开销stack 包括malloc的头部信息 以及 sizeof(Chunk)
        tiny_bytes = 256 - slack_bytes, // 极小块的可使用的大小
        small_bytes = 1 * K - slack_bytes, //小块的可使用的大小
        medium_bytes = 10 * K - slack_bytes, // 中等块的可使用的大小
        large_bytes = 32 * K - slack_bytes, // 大块的可使用的大小
    };
    /**
     *
     * @param actual_bytes 实际的整个内存块的大小 包括头部
     */
    explicit ArenaChunk(size_t actual_bytes);


    /**
     * 可使用的内存的起始位置
     * 申请到了地址 +  内存快Chunk头部的开销 == 可用的头部地址
     * @return
     */
    [[nodiscard]] inline auto bottom_literal() const {
        return (uintptr_t) (this) + sizeof(ArenaChunk);
    };

    /**
     * 可使用内存的结束位置 不包括这个地址
     * @return
     */
    [[nodiscard]] inline auto end_literal() const {
        return this->_end_literal;
    };

    /**
     * 下一个可用的内存块
     * 如果没有 应该返回nullptr
     * @return
     */
    [[nodiscard]] inline ArenaChunk *next() const { return _next; };

    inline void set_next(ArenaChunk *segment) { _next = segment; };

    [[nodiscard]] inline size_t length() const {
        return this->end_literal() - this->bottom_literal();
    };

    /**
     * 指针是否包含在Chunk之中
     * @param p 指针
     * @return
     */
    bool contains(void *p) const;


    /**
     * 如果是标准的尺寸 会首先从ChunkPool中申请
     * 申请不到 才会调用 malloc
     * 非标准尺寸 直接调用malloc
     * @param size
     * @param fail_mode
     * @param len
     * @return
     */
    void *operator new(size_t size, size_t len, bool exit_oom);

    /**
     * 如果是标准的尺寸 那么会归还到ChunkPool
     * 非标准尺寸 直接调用free
     * @param p
     */
    void operator delete(void *p);
};


#endif //PLAT_ARENA_CHUNK_HPP
