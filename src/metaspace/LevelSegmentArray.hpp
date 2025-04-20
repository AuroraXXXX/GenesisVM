//
// Created by aurora on 2022/12/28.
//

#ifndef METASPACE_LEVEL_SEGMENT_ARRAY_HPP
#define METASPACE_LEVEL_SEGMENT_ARRAY_HPP

#include "platform/utils/LinkList.hpp"
#include "SegmentLevel.hpp"
#include "Segment.hpp"
namespace metaspace {
    class Segment;

    /**
     * 用于管理不同内存块等级的数组
     */
    class LevelSegmentArray {
    public:
        using Node = LinkList<Segment>;
    private:
        /**
         * 管理整个空闲内存块的链表
         * 用于把增加 删除的逻辑抽象出来
         *
         * 每个等级中插入顺序
         * 按照提交内存从大到小
         */
        Node _list_array[SegmentLevel::LV_NUM];
        size_t _num_segments_at_level[SegmentLevel::LV_NUM];
        /**
         * 获取对应内存块级别的 管理链表
         * @param level 对应的内存块级别
         * @return
         */
        [[nodiscard]] inline Node *list_for_level(SegmentLevel_t level) const {
            return const_cast<Node *>(this->_list_array + level);
        };
        /**
         * 获取对应内存块级别的 数量地址
         * @param level 对应的内存块级别
         * @return
         */
        [[nodiscard]] inline auto num_segments_for_level(SegmentLevel_t level) const {
            assert(SegmentLevel::is_valid(level), "segment level is invalid");
            return this->_num_segments_at_level + level;
        };
    public:
        explicit LevelSegmentArray();

        /**
         * 将内存块插入
         * @param segment 内存块地址
         */
        void add(Segment *segment);

        /**
         * 将内存块删除
         * @param chunk
         */
        void remove(Segment *segment);

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
         * 统计在固定等级上的内存块所有已提交的大小
         * @param level
         * @return
         */
        [[nodiscard]] size_t calculate_committed_bytes_at_level(SegmentLevel_t level) const;

        /**
         * 采用内存块等级降序的顺序 搜寻符合条件的
         * 即从小内存块(level等级)找到最大(根块)
         * @param level
         * @param min_committed_bytes
         * @return
         */
        Segment *search_segment_descending(SegmentLevel_t level,
                                           size_t min_committed_bytes);

        /**
         * 采用内存块等级升序的顺序 搜寻符合条件的
         * 即从大内存块(level等级)找到最小的内存块(max_level等级)
         * @param level
         * @param max_level 内存块最大等级
         * @param min_committed_bytes
         * @return
         */
        Segment *search_segment_ascending(SegmentLevel_t level,
                                          SegmentLevel_t max_level,
                                          size_t min_committed_bytes);

        /**
         * 输出节点信息
         * @param out
         */
        void print_on(CharOStream *out) const;
        /**
         * 判断当前块是不是被包含
         * @param segment
         * @return
         */
        bool contain(Segment *segment);
    };
}


#endif //METASPACE_LEVEL_SEGMENT_ARRAY_HPP
