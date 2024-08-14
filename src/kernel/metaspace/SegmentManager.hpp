//
// Created by aurora on 2022/12/28.
//

#ifndef KERNEL_METASPACE_SEGMENT_MANAGER_HPP
#define KERNEL_METASPACE_SEGMENT_MANAGER_HPP

#include "kernel/utils/LinkedList.hpp"
#include "kernel/metaspace/constants.hpp"
#include "kernel/metaspace/constants.hpp"

namespace metaspace {
    class Segment;

    /**
     * 用于管理不同内存块等级的数组
     */
    class SegmentManager {
    private:
        /**
         * 管理整个空闲内存块的链表
         * 用于把增加 删除的逻辑抽象出来
         *
         * 存在提交内存的 插入到头部
         * 不存在的 插入到尾部
         */
        LinkList<Segment> _list_array[(SegementLevel_t)SegmentLevel::LV_NUM];
        size_t _num_segments_at_level[(SegementLevel_t)SegmentLevel::LV_NUM];

        [[nodiscard]] inline LinkList<Segment> *list_for_level(SegmentLevel level) const {
            return (LinkList<Segment> *) (this->_list_array + (SegementLevel_t)level);
        };
    public:
        explicit SegmentManager();

        /**
         * 将内存块插入
         * @param chunk
         */
        void add(Segment *segment);

        /**
         * 将内存块删除
         * @param chunk
         */
        void remove(Segment *segment);

        [[nodiscard]] Segment *first_at_level(SegmentLevel level) const {
            return this->list_for_level(level)->head();
        };

        /**
         * 在等级上的内存块数量
         * @param level
         * @return
         */
        [[nodiscard]] inline auto num_segments_at_level(SegmentLevel level) const {
            assert(level_is_valid(level), "segment level is invalid");
            return this->_num_segments_at_level[(SegementLevel_t)level];
        };

        /**
         * 统计所有的内存块数量
         * @return
         */
        [[nodiscard]] size_t num_segments() const;

        /**
         * 统计所有的空闲块保留的地址空间大小
         * @return
         */
        [[nodiscard]] size_t total_bytes() const;

        /**
         * 统计在固定等级上的内存块已提交的大小
         * @param level
         * @return
         */
        [[nodiscard]] size_t calculate_committed_bytes_at_level(SegmentLevel level) const;

        /**
         * 采用内存块等级降序的顺序 搜寻符合条件的
         * 即从小内存块(level等级)找到最大(根块)
         * @param level
         * @param min_committed_bytes
         * @return
         */
        Segment *search_segment_descending(SegmentLevel level,
                                           size_t min_committed_bytes);

        /**
         * 采用内存块等级升序的顺序 搜寻符合条件的
         * 即从大内存块(level等级)找到最小的内存块(max_level等级)
         * @param level
         * @param max_level 内存块最大等级
         * @param min_committed_bytes
         * @return
         */
        Segment *search_segment_ascending(SegmentLevel level,
                                          SegmentLevel max_level,
                                          size_t min_committed_bytes);

        /**
         * 输出节点信息
         * @param out
         */
        void print_on(CharOStream *out) const;

        bool contain(Segment *segment);
    };
}


#endif //KERNEL_METASPACE_SEGMENT_MANAGER_HPP
