//
// Created by aurora on 2022/12/16.
//

#ifndef METASPACE_SEGMENT_HPP
#define METASPACE_SEGMENT_HPP

#include "platform/typedef.hpp"
#include "SegmentLevel.hpp"
#include "platform/utils/LinkList.hpp"
#include "platform/utils/robust.hpp"
#include "platform/utils/align.hpp"

namespace metaspace {
    class Volume;

    /**
     * 内存块的有效负载(即覆盖的内存)可能已提交 部分提交 完全未提交
     *              +--------------+ <- end    -----------+ ----------+
     *              |              |                      |           |
     *              |              |                      |           |
     *              |              |                      |           |
     *              |              |                      |           |
     *              |              |                      |           |
     *         ---- | -----------  | <- committed_top  -- +           |
     *              |              |                      |           |
     *              |              |                      | "free"    |
     *              |              |                      |           | size
     *              |              |    "已提交但未使用"    |           |
     *  committed   |              |                      |           |
     *              |              |                      |           |
     *              | -----------  | <- top     --------- + --------  |
     *              |              |                      |           |
     *              |              |     "已使用"          |           |
     *              |              |                      |           |
     *         ---- +--------------+ <- start   ----------+ ----------+
     */
     class SegmentBase{
     private:


     };

    class Segment :public LinkListNode<Segment> {
        friend class RootArea;
    public:
        /**
         * 表示当前块的状态
         * InUse 表示当前内存块 正在被用户使用
         * Free 表示当前内存块 空闲的，仍持有其管理的内存块
         * Dead 表示当前内存块已经被释放，
         *
         */
        enum class State : uint8_t {
            InUse,
            Free,
            Dead
        };
    private:
        /**
         * 内存块等级
         */
        SegmentLevel_t _level;
        /**
         * 内存块状态
         */
        State _state;
        /**
         * 隶属于的虚拟节点
         */
        Volume *_container;
        /**
         * 这两个指针是固定的
         * 指向地址空间分配时候 虚拟节点中MetaChunk的关系
         * 用于内存块的合并和切分
         */
        LinkListNode<Segment> _buddy_link_node;

    public:
        inline auto state(){
            return this->_state;
        };

        /**
         * 获取当前内存块所属的虚拟节点
         * @return
         */
        [[nodiscard]] inline Volume *container() const {
            return this->_container;
        };

        /**
         * 获取内存块管理的字节数
         * @return
         */
        [[nodiscard]] inline size_t total_bytes() const {
            return SegmentLevel::get_bytes(this->_level);
        };

        [[nodiscard]] inline auto level() const {
            return this->_level;
        };

        /**
         * 判断当前块是不是根块
         * @return
         */
        [[nodiscard]] inline bool is_root_segment() const {
            return this->_level == SegmentLevel::LV_ROOT;
        };
    private:
        /**
         * 管理的内存首地址
         */
        uintptr_t _base;

        /**
         * 已经使用的内存大小
         */
        size_t _used_bytes;

        /**
         * 提交的内存大小
         */
        size_t _committed_bytes;

        /**
         * 将内存边界向上调整
         * @param new_commit_bytes 新的提交内存边界
         * @return false 表示达到了限制,提交失败了
         */
        bool commit_up_to(size_t new_commit_bytes);


    public:


        /**
         * 提供给ChunkHeaderPool
         */
        explicit Segment();


        /**
         * 获取状态对应的字符
         * 死亡状态(Dead)   使用 D
         * 空闲状态(Free)   使用 F
         * 使用状态(InUse)  使用 U
         * @return
         */
        [[nodiscard]]  char get_state_char() const;


        /**
         * 已经使用的内存 不包括内存块的开销
         * @return
         */
        [[nodiscard]] inline size_t used_bytes() const {
            return this->_used_bytes;
        };

        /**
         * 获取已经提交的内存
         * @return
         */
        [[nodiscard]] inline size_t committed_bytes() const {
            return this->_committed_bytes;
        };

        /**
         * 获取当前
         * @return
         */
        [[nodiscard]] size_t free_bytes() const {
            return this->total_bytes() - this->used_bytes();
        };

        /**
         * 判断已经提交的内存中 剩余可用的内存
         * @return
         */
        [[nodiscard]] inline size_t free_below_committed_bytes() const {
            return this->_committed_bytes - this->_used_bytes;
        };


        /**
         * 如果这个segment是它的buddy对中的leader，则返回true，否则返回false。不要调用根块。
         * @return
         */
        [[nodiscard]] inline bool is_leader() const {
            assert(!this->is_root_segment(), "root segment does not have partner ");
            return is_aligned(
                    this->_base,
                    SegmentLevel::get_bytes(this->_level));
        };

        /**
         * 在提交内存的限制下 分配大小
         * @param request_bytes 需求大小 必须对齐
         * @return 无法分配时返回空
         */
        void *allocate(size_t request_bytes);

        /**
         * 确保已提交内存中 未被分配出去内存，满足需求
         * 当不足时 会获取元空间锁  进行新的提交
         * 共
         * @param new_commit_bytes 需求的内存
         * @return
         */

        bool ensure_committed_enough(size_t new_commit_bytes);

        /**
         * 将整个内存块的内存撤销提交
         * 这个必须已经获取元空间锁才可以调用
         */
        void clear_committed();

        /**
         * 打印当前节点的信息
         * @param out
         */
        void print_on(CharOStream *out) const;
    };
}


#endif //METASPACE_SEGMENT_HPP
