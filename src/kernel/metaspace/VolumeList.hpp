//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_VOLUME_LIST_HPP
#define KERNEL_METASPACE_VOLUME_LIST_HPP

#include "plat/mem/allocation.hpp"

namespace metaspace {
    class Volume;
    class Segment;

    /**
     * 虚拟空间节点链表
     * 这里面所有的方法全部都由 ChunkManager中调用的
     */
    class VolumeList : public CHeapObject<MEMFLAG::Metaspace> {
    private:
        Volume *_list_head;
        /**
         * 当前节点链表 持有的虚拟节点数量
         */
        size_t _list_length;
        /**
         * 整个虚拟空间节点链表 保留下来的虚拟空间大小
         */
        size_t _reserved_bytes;
        /**
         * 整个虚拟空间节点链表 提交的实际大小
         */
        size_t _committed_bytes;


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
            return this->_reserved_bytes;
        };

        /**
         * 统计 整个虚拟节点的已提交的内存大小
         * @return
         */
        [[nodiscard]] inline size_t committed_bytes() const {
            return this->_committed_bytes;
        };

        [[nodiscard]] inline auto volumes_num() const {
            return this->_list_length;
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
         * 分配一个根块
         * @return 失败 nullptr
         */
        Segment *allocate_root_segment();

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
#ifdef DIAGNOSE
        void verify();
#endif
    };
}

#endif //KERNEL_METASPACE_VOLUME_LIST_HPP
