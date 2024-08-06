//
// Created by aurora on 2022/12/19.
//

#ifndef KERNEL_METASPACE_BLOCK_TREE_HPP
#define KERNEL_METASPACE_BLOCK_TREE_HPP

#include "plat/mem/allocation.hpp"

namespace metaspace {
    /**
     *                   +-----+
     *                   | 100 |
     *                   +-----+
     *                  /       \
     *           +-----+
     *           | 80  |
     *           +-----+
     *          /   |   \
     *         / +-----+ \
     *  +-----+  | 80  |  +-----+
     *  | 70  |  +-----+  | 85  |
     *  +-----+     |     +-----+
     *           +-----+
     *           | 80  |
     *           +-----+
     */
    class BlockTree : public CHeapObject<MEMFLAG::Management> {
        /**
         * 如果需要被MetaTree管理的内存块,那么需要转换成Node
         */
        class Node {
        public:
            Node *_left;
            Node *_right;
            Node *_parent;
            Node *_next;
            const size_t _bytes;

            explicit Node(size_t bytes) :
                    _bytes(bytes),
                    _next(nullptr),
                    _left(nullptr),
                    _right(nullptr),
                    _parent(nullptr) {};

        };

    public:
        constexpr inline static size_t MIN_BYTES = sizeof(Node);
    private:
        Node *_root;
        size_t _total_bytes;

        /**
         * 寻找最贴近的节点
         * @param bytes 内存大小
         * @return
         */
        Node *find_closest_node(size_t bytes);

        /**
         * 使用replace节点代替child节点
         * @param child
         * @param replace
         */
        void replace_node_in_parent(Node *child, Node *replace);

        /**
         * 将child设置为parent的左孩子节点
         * @param parent
         * @param child
         */
        static void set_left_child(Node *parent, Node *child);

        static void set_right_child(Node *parent, Node *child);

        /**
         * 将node内存块添加到head节点之后
         * @param node
         * @param head
         */
        static void add_to_list(Node *node, Node *head);

        /**
         * 删除head这个节点 内存块链表的 的第二块内存
         * 也就是这个节点 必须存在至少两个内存块
         * @param head
         * @return
         */
        static Node *remove_from_list(Node *head);

        /**
         * 寻找node节点的后继节点 ，即下一个比node大的内存节点
         * @param node
         * @return
         */
        static Node *successor(Node *node);

        void remove_node_from_tree(Node *node);

        static void insert(Node *insertion_point, Node *n);

    public:
        explicit BlockTree() : _root(nullptr), _total_bytes(0) {};

        /**
         * 将内存添加到二叉树中
         * @param p 内存的首地址
         * @param bytes 内存的大小 单位字节
         */
        void add_meta_node(void* p, size_t bytes);

        /**
         * 将删除至少是bytes大小的节点 并返回
         * @param bytes 需求的内存答案小
         * @param real_bytes 实际的内存大小
         * @return 内存首地址
         */
        void* remove_meta_node(size_t bytes, size_t *real_bytes);

        inline bool is_empty() { return this->_root == nullptr; };

        [[nodiscard]] inline size_t total_bytes() const { return this->_total_bytes; };
    };
}

#endif //KERNEL_METASPACE_BLOCK_TREE_HPP
