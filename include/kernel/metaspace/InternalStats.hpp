//
// Created by aurora on 2022/12/24.
//

#ifndef KERNEL_METASPACE_INTERNAL_STATS_HPP
#define KERNEL_METASPACE_INTERNAL_STATS_HPP

#include "atomic"
#include "plat/mem/AllStatic.hpp"
#include "plat/macro.hpp"

class CharOStream;

namespace metaspace {
    /**
     * 统计元空间状态信息
     * 用于分析和调试
     */
    class InternalStats : public AllStatic {
    /**
     * 只要不在Expand_Lock保护下的
     * 参数必须是原子类型的
     * x 表示这个参数是非原子类型的
     * x_atomic 表示这个参数必须是原子类型的
     *
     * 但是这些信息仅仅用于给人员肉眼观察
     * 而不是用于元空间内部的调整
     */
#define ALL_INTERNAL_STATS(x, x_atomic)                                             \
    DEBUG_MODE_ONLY(x_atomic(num_allocs,"成功内存分配次数"))                           \
    /**释放元空间内存的次数，即执行MetaspaceArena::deallocate()次数*/                     \
    DEBUG_MODE_ONLY(x_atomic(num_deallocs,"内存释放次数"))                             \
    /**从BlockManager中获取内存块的次数*/                                          \
    DEBUG_MODE_ONLY(x_atomic(num_allocs_from_blocks_manager,"从已释放的块中满足分配的次数"))      \
    /**Arena::salvage_current_chunk*/                                  \
    DEBUG_MODE_ONLY(x_atomic(num_segments_retire,"退役正在使用内存块的次数"))                           \
    x_atomic(num_allocs_failed_limit,"由于触发限制,内存分配失败次数")                \
                                                                                \
    /**统计MetaspaceArena的存活和销毁的数量*/                                       \
    x_atomic(num_arena_births,"存活的arena数量")                                  \
    x_atomic(num_arena_deaths,"死亡的arena数量")                                  \
    /**--------------global stat-----------------*/                             \
    /**统计Volume的存活和销毁的数量*/                                               \
    x(num_volumes_births,"存活的vsnode数量")                                      \
    x(num_volumes_deaths,"死亡的vsnode数量")                                      \
    /**统计commit_range*/                                                        \
    x(num_range_committed,"提交内存区间的次数")                                    \
    /**统计uncommit_range*/                                                      \
    x(num_range_uncommitted,"撤销提交内存区间的次数")                               \
                                                                                \
    /**统计来自于ChunkManager::return_chunk*/                                     \
    x(num_segments_to_manager,  "从SegmentManager中归还的segment数量")             \
    /**统计来自于ChunkManager::get_chunk*/                                        \
    x(num_segments_from_manager,"从SegmentManager中获取的segment数量")            \
                                                                                \
    /**统计来自于ChunkManager::attempt_merge_chunk*/                              \
    x(num_segments_merges,"成功的块合并数量")                                        \
    /**统计来自于ChunkManager::split_chunk*/                                      \
    x(num_segments_splits,"块分割数量")                                             \
    /**统计来自于ChunkManager::attempt_enlarge_chunk*/                            \
    x(num_segments_enlarged,"适当放大块的数量")                                      \
                                                                                \
    x(num_arena_inconsistent_stat,"读取状态的次数")

    private:
#define DEFINE_COUNTER(name, human) static std::atomic<uint64_t> _##name;
#define DEFINE_ATOMIC_COUNTER(name, human) static uint64_t _##name;
        ALL_INTERNAL_STATS(DEFINE_COUNTER, DEFINE_ATOMIC_COUNTER)
#undef DEFINE_ATOMIC_COUNTER
#undef DEFINE_COUNTER
    public:
        /**
         * 用于增加相应的计数
         */
#define INCREMENTOR(name, human) static inline void inc_##name(){++ _##name;};

        ALL_INTERNAL_STATS(INCREMENTOR, INCREMENTOR)
#undef INCREMENTOR
        /**
         * 获取参数的函数
         */
#define GETTER(name, human) static inline uint64_t name(){return _##name;};

        ALL_INTERNAL_STATS(GETTER, GETTER)
#undef GETTER

        /**
         * 打印元空间的状态信息
         * @param out
         */
        static void print_on(CharOStream *out);
    };
}

#endif //KERNEL_METASPACE_INTERNAL_STATS_HPP
