//
// Created by aurora on 2022/12/16.
//

#ifndef METASPACE_VOLUME_LIST_HPP
#define METASPACE_VOLUME_LIST_HPP

#include "platform/allocation.hpp"
#include "platform/utils/LinkStack.hpp"
#include <atomic>

namespace metaspace {
    class Volume;
    class Segment;

    /**
     * 虚拟空间节点链表
     * 这里面所有的方法全部都由 ChunkManager中调用的
     */
    class VolumeList : public CHeapObject<MEMFLAG::Metaspace> {
    private:
        LinkStack<Volume> _list;
        /**
         * 整个虚拟空间节点链表 保留下来的虚拟空间大小
         */
        std::atomic<size_t> _reserved_bytes;
        /**
         * 整个虚拟空间节点链表 提交的实际大小
         */
        std::atomic<size_t> _committed_bytes;


        /*
         * 但是_can_expand必须时false
         * 创建一个新的虚拟节点
         */
        void create_new_volume();

    public:
        /**
         * 保留的地址空间大小
         * @return
         */
        [[nodiscard]] inline size_t reserved_bytes() const {
            return this->_reserved_bytes.load();
        };

        /**
         * 统计 整个虚拟节点的已提交的内存大小
         * @return
         */
        [[nodiscard]] inline size_t committed_bytes() const {
            return this->_committed_bytes.load();
        };


        /**
         * 直接初始化虚拟节点链表
         * @param name 虚拟节点的名称
         */
        explicit VolumeList();

        /**
         * 应在获取元空间锁的情况下才可以进行
         * 销毁整个虚拟节点链表
         */
        ~VolumeList();

        /**
         * 打印本链表的信息
         * 内部应该先获取锁
         * @param out
         */
        void print_on(CharOStream *out) const;

        /**
         * 判断地址是否在虚拟节点中
         * @param p
         * @return
         */
        bool contains(void* p) const;
#ifdef DEBUG_MODE_ONLY
        void verify();
#endif
    };
}

#endif //METASPACE_VOLUME_LIST_HPP
