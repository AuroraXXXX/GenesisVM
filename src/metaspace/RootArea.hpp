//
// Created by aurora on 2025/3/25.
//

#ifndef METASPACE_ROOT_AREA_HPP
#define METASPACE_ROOT_AREA_HPP

#include "platform/typedef.hpp"
#include "platform/utils/LinkList.hpp"
#include "Segment.hpp"

namespace metaspace {
    class Volume;

    class LevelSegmentArray;

    /**
     * 用于管理一块内存块，内部可以且分出Segment。本方法也用于Segment分割与合并
     */
    class RootArea {

    private:
        /**
         * 隶属于的volume
         */
        Volume *_volume;
        /**
         * 该Volume中可以且分出RootSegment的数量
         */
        uint32_t _total;
        /**
         * 下一个可用的
         */
        uint32_t _top;
        /**
         * 用于管理每个根块的伙伴节点
         * 主要用于，最后的RootSegmentHeader的释放
         */
        Segment *_segments[0];

        /**
         * 构造函数
         * @param volume 所属的Volume
         * @param total Volume中可以且分出RootSegment的数量
         */
        explicit RootArea(Volume *volume, uint32_t total);

    public:
        /**
          * 构造函数
          * @param volume 所属的Volume
          * @param total Volume中可以且分出RootSegment的数量
          */
        static RootArea *create(Volume *volume, uint32_t total);

        ~RootArea();

        /**
         * 进行合并
         * @param segment
         * @param array
         * @return
         */
        static Segment *merge(Segment *segment, LevelSegmentArray *array);

        /**
         * 将指定的segment切分，直到segment 的level 为 target_level
         * @param segment
         * @param target_level
         * @param array
         */
        static void split(Segment *segment,
                          SegmentLevel_t target_level,
                          LevelSegmentArray *array);

        /**
         * 获取本内存块的长度
         * @return
         */
        [[nodiscard]] uint32_t total() const {
            return this->_total;
        };

        Segment* alloc_root_segment();

    };
}


#endif //METASPACE_ROOT_AREA_HPP
