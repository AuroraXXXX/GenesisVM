//
// Created by aurora on 2022/4/12.
//

#ifndef PLATFORM_ARENA_HPP
#define PLATFORM_ARENA_HPP

#include "allocation.hpp"
class ArenaChunk;

/**
 * 支持快速分配内存的对象
 *
 * 内存分配的策略是
 * 如果当前块(Chunk)无法满足要求
 * 那么当前块(Chunk)的剩余内存全部抛弃,被视为已经使用
 *
 */
class Arena : public CHeapObject<MEMFLAG::Arena> {
    friend class ArenaMark;
private:
    /**
     * 用于标记区分快速内存的类别
     */
    const MEMFLAG _flag;
    /**
     * 分别指向Chunk组成的链表头部和尾部
     * 新添加的内存块会放入到链表的尾部
     */
    ArenaChunk *_head;
    ArenaChunk *_tail;
    /**
     * 在_hwm 到 _max之间的内存是 是没有被分配出去的
     */
    uintptr_t _top_literal;
    uintptr_t _end_literal;
    /**
     * 当前已经管理的内存大小
     * 包括申请还没有使用的
     */
    size_t _total_bytes;

    /**
     * 检查是否溢出
     * @param request 请求的大小
     * @param exit_oom 申请失败的退出虚拟机
     * @return  true    表示溢出
     *          false   表示没有溢出
     */
    [[nodiscard]] bool check_overflow(
            size_t request,
            bool exit_oom) const;


    void new_chunk(size_t chunk_bytes, bool exit_oom);

    /**
     * 删除chunk节点和之后的所有节点
     * @param chunk
     * @return 删除的长度
     */
    size_t chop_list(ArenaChunk *chunk) const;

public:
    class ChunkClosure {
    public:
        /**
         * 用于迭代内部的内存块
         * @param bottom 内存块中可以分配的首地址
         * @param top 内存块中分配结束的地址
         */
        virtual void do_chunk(void *base, void *top) = 0;
    };

    class SavedData {
        friend class Arena;

    private:
        ArenaChunk *_tail;
        uintptr_t _top_literal;
        uintptr_t _end_literal;
        size_t _total_bytes;
    public:
        explicit SavedData(Arena *arena);

        void rollback_to(Arena *arena);
    };

    /**
     * 遍历内存块
     * @param func
     */
    void iter_chunk(ChunkClosure *closure);

    [[nodiscard]] inline MEMFLAG flag() const { return _flag; };

    /**
     *
     * @param F 快速内存的类型
     * @param init_bytes 初始的第一块Chunk的大小
     */
    explicit Arena(MEMFLAG F, size_t init_bytes);

    explicit Arena(MEMFLAG F);

    /**
     * 调用析构函数 释放所持有的Chunk
     */
    ~Arena();

    /**
     * 内存申请函数
     * @param request 对齐到 align
     * @param exit_oom 内存申请失败时退出虚拟机
     * @return
     */
    void *alloc(size_t request, bool exit_oom);

    /**
     * 快速内存释放函数
     *  释放的条件较为苛刻
     *  仅仅上一次申请的内存 才会真正的释放
     *  并且也仅能释放当前内存块的
     * @param ptr 申请时候获取的首地址
     * @param request 申请的大小
     * @return true 表示内存释放掉了
     *         false 表示内存并没有释放掉
     */
    bool free(void *ptr, size_t request);

    /**
     * 清除全部的内存池
     */
    static void clean_pool();
};


#endif //PLATFORM_ARENA_HPP
