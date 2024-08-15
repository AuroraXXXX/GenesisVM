//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_ARENA_HPP
#define KERNEL_METASPACE_ARENA_HPP

#include "plat/mem/allocation.hpp"
#include "kernel/utils/LinkedList.hpp"
#include "kernel/metaspace/ArenaGrowthPolicy.hpp"

class Mutex;
namespace metaspace {
    class Segment;

    class ArenaGrowthPolicy;

    class BlockManager;

    /**
     * 元空间内存的分配对象
     */
    class Arena : public CHeapObject<MEMFLAG::Metaspace> {
    private:

        BlockManager *_block_manager;
        /**
         * 管理已经使用的内存块
         */
        LinkList<Segment> _segments;
        /**
         * 扩展的策略
         */
        ArenaGrowthPolicy *_policy;
        /**
         * 链表中内存块的数量
         */
        size_t _num_of_segments;


        /**
         * 满足一定的条件 才会真正的尝试扩展当前块
         * @param need_bytes 需求的字节数 不要求对齐
         * @return 扩展是否成功
         */
        bool attempt_enlarge_current_segment(size_t need_bytes);


        /**
         * 从当前的BlockManager中申请
         * @param need_bytes 需求的内存 已经与元空间申请的对齐边界对齐
         * @return
         */
        void* allocate_from_block(size_t need_bytes);

        /**
         * 尝试从当前块中申请
         * @param need_bytes
         * @return
         */
        void* allocate_from_current_segment(size_t need_bytes);

        /**
         * 从新块中再次执行申请
         * @param need_bytes
         * @return
         */
        void* allocate_from_new_segment(size_t need_bytes);

        /**
         * 申请一个新的块 并且确保新的块中 已提交内存可以满足need_bytes
         * @param need_bytes 用户需求的内存大小
         * @return
         */
        Segment *create_new_segment(size_t need_bytes);

        /**
         * 将正在使用的内存块剩余内存回收掉，
         * @param segment
         */
        void salvage_segment(Segment *segment);

        /**
         * 正在使用的内存块
         * @return
         */
        inline Segment *current_use_segment() {
            return const_cast<Segment *>(this->_segments.head());
        };

        inline SegmentLevel policy_suggest_next_level() {
            return this->_policy->get_level_by_step(this->_num_of_segments);
        };
    public:
        /**
         * 向元空间申请一段内存
         * 1 尝试从BlockManager(管理已释放内存块的字典)中 进行获取
         * 2 尝试从当前的内存块进行获取
         * 3 当前块太小时，对当前快进行扩展 (扩展至少是当前块的两倍)
         * 4 尝试获取新的块，并从该块进行分配(在获取新块的时候,应该将旧的内存块剩余的已提交内存
         * 放入 管理已释放内存块的字典中)
         * 在任何时候，如果达到内存提交的限制 则返回空nullptr
         * @param required_bytes 需求的内存
         * @return 内存首地址
         */
        void* allocate(size_t required_bytes);

        /**
         * 归还元空间内存
         * @param p
         * @param bytes
         */
        void deallocate(void* p, size_t bytes);

        /**
         *
         * @param policy 策略
         */
        explicit Arena(ArenaGrowthPolicy *policy);

        /**
         * 析构函数
         * 销毁 BlockManager
         */
        ~Arena();

        /**
         * 粗略统计内部内存块的使用情况
         * @param used_bytes 已使用的内存大小
         * @param committed_bytes  已提交内存大小
         * @param capacity_bytes total内存容量
         */
        void usage_numbers(size_t *used_bytes,
                           size_t *committed_bytes,
                           size_t *capacity_bytes);

        void print_on(CharOStream *stream);
    };
}


#endif //KERNEL_METASPACE_ARENA_HPP
