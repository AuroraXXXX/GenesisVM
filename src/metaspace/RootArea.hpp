//
// Created by aurora on 2025/3/25.
//

#ifndef METASPACE_ROOT_AREA_HPP
#define METASPACE_ROOT_AREA_HPP

#include "platform/typedef.hpp"
#include "platform/utils/LinkList.hpp"
namespace metaspace{
    class Volume;
    class Segment;
    /**
     * 用于管理一块内存块，内部可以且分出Segment。本方法也用于Segment分割与合并
     */
    class RootArea {
    private:
        Volume* _volume;
        uint32_t _total;
        uint32_t _top;
        /**
         * 用于管理每个根块的伙伴节点
         */
        LinkList<Segment> _segments[0];
        /**
         * 构造函数
         * @param volume 所属的Volume
         * @param total Volume中可以且分出RootSegment的数量
         */
        explicit RootArea(Volume* volume, uint32_t total);
    public:
        /**
          * 构造函数
          * @param volume 所属的Volume
          * @param total Volume中可以且分出RootSegment的数量
          */
        static RootArea* create(Volume* volume,uint32_t total);



        /**
         * 获取本内存块的长度
         * @return
         */
        [[nodiscard]] uint32_t total() const{
            return this->_total;
        };
    };
}


#endif //METASPACE_ROOT_AREA_HPP
