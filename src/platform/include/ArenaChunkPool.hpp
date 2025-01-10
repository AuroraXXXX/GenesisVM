//
// Created by aurora on 2023/10/8.
//

#ifndef PLAT_ARENA_CHUNK_POOL_HPP
#define PLAT_ARENA_CHUNK_POOL_HPP

#include "plat/mem/allocation.hpp"
#include "plat/utils/robust.hpp"

class ArenaChunk;

/**
 * 持有快速内存中空闲的内存池
 * 每个内存池仅仅持有统一大小的内存块
 */
class ArenaChunkPool : public CHeapObject<MEMFLAG::Internal> {
private:
    /**
     * 内存池 所持有的 快速内存块 链表
     */
    ArenaChunk *_list_head;
    /**
     * 下面四个是不同尺寸的内存池
     */
    static ArenaChunkPool *_large_pool;
    static ArenaChunkPool *_medium_pool;
    static ArenaChunkPool *_small_pool;
    static ArenaChunkPool *_tiny_pool;


    /**
     * 内存池的构造函数
     */
    explicit ArenaChunkPool() :
            _list_head(nullptr) {};

    /**
     * 仅仅当内存池缓存的内存块数量
     * > 这个数值chunk_to_keep的时候
     * 才会把内存池中多余的内存块释放除去
     *
     * 线程安全的
     * @param chunk_to_keep 内存池可以保持的内存块数量
     */
    void purge();

    inline ~ArenaChunkPool() {
        this->purge();
    };
public:
    /**
     * 获取指定大小的内存池 ，获取不到的返回 nullptr
     * @param chunk_bytes 实际的可用空间
     * @return
     */
    static ArenaChunkPool *get_pool(size_t chunk_bytes);

    /**
     * 从内存池中申请得到一个内存块
     * 线程安全的
     * @return
     */
    ArenaChunk *alloc();

    /**
     * 将内存块归还到ChunkPooL中 进行缓存起来
     *
     * 线程安全的
     * @param chunk 内存块
     */
    void free(ArenaChunk *chunk);

    /**
     * 初始化内存池
     */
    static void initialize();

    /**
     * 在GC结束后 包括YoungGC
     * 但是并不会直接清理完毕所有空闲的内存块
     * 因为这个是快速内存 可能需要频繁申请
     * 内部并没有上锁
     * 所以需要
     */
    static void clean();

};


#endif //PLAT_ARENA_CHUNK_POOL_HPP
