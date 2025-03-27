//
// Created by aurora on 2025/3/27.
//

#ifndef PLATFORM_ARENA_MARK_HPP
#define PLATFORM_ARENA_MARK_HPP
#include "platform/allocation.hpp"
#include "platform/macro.hpp"
class Arena;
class ArenaChunk;

/**
 * OSThread 的 resource mem
 */
class ArenaMark : public StackObject {
    friend class Arena;
private:
    /**
     * 保存的Arena栈顶指针
     */
    ArenaChunk *_current_top;
    /**
     * 记录当前正在使用的ArenaChunk的 top 指针
     */
    uintptr_t _top_literal;
    /**
     * 记录当前正在使用的ArenaChunk的 end 指针
     */
    uintptr_t _end_literal;
    /**
     * 记录当前正在使用的ArenaChunk的容量
     */
    size_t _total_bytes;
    /**
     * 记录要操作的arena
     */
    DEBUG_MODE_ONLY( Arena* _arena;)
public:
    /**
     * 记录保存点
     * @param arena
     */
    explicit ArenaMark(Arena* arena);
    /**
     * 进行回滚操作
     * @param arena
     */
    void rollback_to(Arena* arena);
};
#endif //PLATFORM_ARENA_MARK_HPP
