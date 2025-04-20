//
// Created by aurora on 2025/3/25.
//

#ifndef GENESISVM_CONTEXTHOLDER_HPP
#define GENESISVM_CONTEXTHOLDER_HPP

#include "platform/mem/AllStatic.hpp"
#include "SegmentLevel.hpp"
#include <atomic>

class Mutex;
namespace metaspace {
    class VolumeList;

    class LevelSegmentArray;

    class Segment;


    /**
     * 元空间上下文管理类
     *
     */
    class ContextHolder : public AllStatic {
    private:
        /**
         * 管理全局的虚拟节点的链表
         */
         VolumeList *_volume_list;
        /**
         * 管理空间的Segment的链表
         */
         LevelSegmentArray *_level_segment_array;
        /**
         * 元空间中用于管理全局的锁
         */
        static Mutex Metaspace_lock;
        /**
         * 全局的提交的内存的大小
         */
        static std::atomic<size_t> _committed_bytes;

        using POLICY_FUNC_T = size_t(*)(size_t);


    public:
        /**
         * 归还 segment
         * @param segment
         */
        static void return_segment(Segment *segment);

        static void global_initialize();

        /**
         * 获取一个segment
         * 如果成功,至少返回一个max_level级别的内存块,
         *  有宽裕条件会返回preferred_level的内存块
         *  且min_committed_bytes字节 保证被提交
         * 内部首先会获取元空间的锁
         *
         * 如果失败,原因:
         * 1)本身是压缩类空间,且保留的虚拟进程地址空间已被使用完毕,无法被扩展
         * 2)达到内存提交的阈值无法提交内存,阈值有GC阈值和MaxMetaspaceSize
         * @param preferred_level  希望的内存块等级
         * @param max_level (最少内存块大小)最大内存块等级
         * @param min_committed_bytes 最少应该被提交的内存大小 单位字节
         * @return
         */
         Segment *get_segment(SegmentLevel_t preferred_level,
                                    SegmentLevel_t max_level,
                                    size_t min_committed_bytes);

        /**
         * 全局的上下文锁
         * @return
         */
        static inline auto global_locker() {
            return &Metaspace_lock;
        };
    };
}


#endif //GENESISVM_CONTEXTHOLDER_HPP
