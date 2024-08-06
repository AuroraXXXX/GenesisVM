//
// Created by aurora on 2022/12/19.
//
#include "BlockTree.hpp"
#include <new>
#include "plat/utils/robust.hpp"
namespace metaspace {


    BlockTree::Node *BlockTree::find_closest_node(size_t bytes) {
        auto search_point = this->_root;
        Node *best_match = nullptr;
        while (search_point) {
            if (search_point->_bytes >= bytes) {
                best_match = search_point;
                if (search_point->_bytes == bytes) {
                    break; // 最好的
                }
                search_point = search_point->_left;
            } else {
                search_point = search_point->_right;
            }
        }
        return best_match;
    }

    void BlockTree::replace_node_in_parent(BlockTree::Node *child, BlockTree::Node *replace) {
        const auto parent = child->_parent;
        if (parent) {
            //存在父亲节点
            if (parent->_left == child) {
                //是父亲节点的左节点
                set_left_child(parent, replace);
            } else {
                set_right_child(parent, replace);
            }
        } else {
            //不存在父节点 那必定是根节点
            assert(child == _root, "必须是");
            this->_root = replace;
            if (replace) {
                //代替节点不为空 那么作为根节点 这个节点的父节点必须是空
                replace->_parent = nullptr;
            }
        }
    }

    void BlockTree::set_left_child(BlockTree::Node *parent, BlockTree::Node *child) {
        parent->_left = child;
        if (child) {
            assert(child->_bytes < parent->_bytes, "健全");
            child->_parent = parent;
        }
    }

    void BlockTree::set_right_child(BlockTree::Node *parent, BlockTree::Node *child) {
        parent->_right = child;
        if (child) {
            assert(child->_bytes > parent->_bytes, "健全");
            child->_parent = parent;
        }
    }

    void BlockTree::add_to_list(BlockTree::Node *node, BlockTree::Node *head) {
        assert(head->_bytes == node->_bytes, "健全");
        node->_next = head->_next;
        head->_next = node;
    }

    BlockTree::Node *BlockTree::remove_from_list(BlockTree::Node *head) {
        assert(head->_next != nullptr, "健全");
        auto node = head->_next;
        head->_next = node->_next;
        return node;
    }

    BlockTree::Node *BlockTree::successor(BlockTree::Node *node) {
        Node *succ = nullptr;
        if (node->_right) {
            //存在右孩子节点 就应该去右子树查找 不停找左子树
            succ = node->_right;
            while (succ->_left) {
                succ = succ->_left;
            }
        } else {
            succ = node->_parent;
            Node *n2 = node;
            //向上找
            while (succ && n2 == succ->_right) {
                n2 = succ;
                succ = succ->_parent;
            }
        }
        return succ;
    }

    void BlockTree::remove_node_from_tree(BlockTree::Node *node) {
        assert(node->_next != nullptr, "被删除的节点存在>1的内存块");
        if (!node->_left || !node->_right) {
            auto replace = node->_left ? node->_left : node->_right;
            replace_node_in_parent(node, replace);
            return;
        }
        //存在两个子节点
        auto succ = this->successor(node);
        assert(succ != nullptr, "后继节点不为空");
        assert(succ->_left == nullptr, "后继节点的左节点必须为空");
        assert(succ->_bytes > node->_bytes, "后继节点内存块应该大于被删除节点");
        auto succ_parent = succ->_parent;
        if (succ_parent == node) {
            //说明后继节点是被删除节点的右孩子
            assert(node->_right == succ, "健全");
            replace_node_in_parent(node, succ);
            set_left_child(succ, node->_left);
        } else {
            assert(succ_parent->_left == succ, "健全");
            set_left_child(succ_parent, succ->_right);
            // and the successor replaces n at its parent
            replace_node_in_parent(node, succ);
            // and takes over n's old children
            set_left_child(succ, node->_left);
            set_right_child(succ, node->_right);
        }

    }


    void BlockTree::insert(Node *insertion_point, Node *n) {
        assert(n->_parent == nullptr, "Sanity");
        for (;;) {

            if (n->_bytes == insertion_point->_bytes) {
                add_to_list(n, insertion_point); // parent stays NULL in this case.
                break;
            } else if (n->_bytes > insertion_point->_bytes) {
                if (insertion_point->_right == nullptr) {
                    set_right_child(insertion_point, n);
                    break;
                } else {
                    insertion_point = insertion_point->_right;
                }
            } else {
                if (insertion_point->_left == nullptr) {
                    set_left_child(insertion_point, n);
                    break;
                } else {
                    insertion_point = insertion_point->_left;
                }
            }
        }
    }

    void BlockTree::add_meta_node(void* p, size_t bytes) {
        assert(bytes >= MIN_BYTES, "内存块大小错误: " SIZE_FORMAT, bytes);
        Node *n = new(p) Node(bytes);
        if (_root == nullptr) {
            _root = n;
        } else {
            insert(_root, n);
        }
        this->_total_bytes += bytes;
    }

    void * BlockTree::remove_meta_node(size_t bytes, size_t *real_bytes) {
        assert(bytes >= MIN_BYTES, "内存块大小错误: " SIZE_FORMAT, bytes);
        auto node = this->find_closest_node(bytes);
        if (node == nullptr) {
            *real_bytes = 0;
            return nullptr;
        }
        assert(node->_bytes >= bytes, "健全");
        if (node->_next) {
            node = remove_from_list(node);
        } else {
            remove_node_from_tree(node);
        }
        *real_bytes = node->_bytes;
        this->_total_bytes -= node->_bytes;
        return reinterpret_cast<void *>(node);
    }


}


