//
// Created by aurora on 2025/3/25.
//

#ifndef GENESISVM_CONTEXTHOLDER_HPP
#define GENESISVM_CONTEXTHOLDER_HPP

#include "platform/mem/AllStatic.hpp"
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
        static VolumeList *_volume_list;
        /**
         * 管理空间的Segment的链表
         */
        static LevelSegmentArray *_level_segment_array;
        /**
         * 元空间中用于管理全局的锁
         */
        static Mutex Metaspace_lock;
        /**
         * 全局的提交的内存的大小
         */
        static std::atomic<size_t> _committed_bytes;
    public:
        /**
         * 归还 segment
         * @param segment
         */
        static void return_segment(Segment *segment);
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
