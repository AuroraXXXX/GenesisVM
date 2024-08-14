//
// Created by aurora on 2024/1/31.
//

#ifndef NUCLEUSVM_LINKEDLIST_HPP
#define NUCLEUSVM_LINKEDLIST_HPP

#include <concepts>

/**
 * 链表节点的定义 需要存在这些函数
 * @tparam T
 */
template<typename T>
//    concept LinkListNode = requires(T *t){
//        std::is_same_v<decltype(t->prev()), T *>;
//        std::is_same_v<decltype(t->next()), T *>;
//        std::is_same_v<decltype(t->set_prev(t)), void>;
//        std::is_same_v<decltype(t->set_next(t)), void>;
//    };
//
//    template<LinkListNode T>
class LinkList {
private:
    T *_head;
    T *_tail;
public:
    explicit LinkList() noexcept:
            _head(nullptr),
            _tail(nullptr) {};

    inline T *head() const {
        return this->_head;
    };

    inline T *tail() const {
        return this->_tail;
    };

    /**
     * 将节点从链表头部添加
     * @param node
     */
    void head_add_to_list(T *node);

    /**
     * 将节点从链表尾部添加
     * @param node
     */
    void tail_add_to_list(T *node);

    /**
     * 将节点node添加到target附近,
     * before为true时，添加到target之前，否则添加到target之后
     * 但是如果target为null,
     * before为true时添加到链表头部，否则添加到链表尾部
     * @param target
     * @param node
     * @param before
     */
    void add_to_list_target(T *target, T *node, bool before = false);

    /**
     * 将节点从链表中删除
     * @param node
     */
    void delete_from_list(T *node);

    /**
     * 从链表头部删除节点
     * @return 删除的节点
     */
    T *delete_from_list_head();

    /**
     * 从链表为尾部删除节点
     * @return 删除的节点
     */
    T *delete_from_list_tail();

    /**
     * 从链表头部开始遍历
     * @param f
     */
    void node_head_do(bool f(T *));

    template<class F>
    void node_head_do(F f);

    /**
     * 从链表尾部开始遍历
     * @param f
     */
    void node_tail_do(bool f(T *));

    template<class F>
    void node_tail_do(F f);

    /**
     * 验证链表指针关系
     * @return
     */
    [[nodiscard]]  bool verify() const;

    /**
     * 判断节点是否在链表中
     * @param t
     * @return
     */
    bool contain(T *t);

    inline bool is_empty() {
        return this->_head == nullptr;
    };

};


template<typename T>
void LinkList<T>::node_head_do(bool f(T *)) {
    auto node = this->_head;
    while (node != nullptr) {
        if (!f(node)) {
            break;
        }
        node = node->next();
    }
}

template<typename T>
template<class F>
void LinkList<T>::node_head_do(F f) {
    auto node = this->_head;
    while (node != nullptr) {
        if (!f(node)) {
            break;
        }
        node = node->next();
    }
}

template<typename T>
void LinkList<T>::node_tail_do(bool f(T *)) {
    auto node = this->_tail;
    while (node != nullptr) {
        if (!f(node)) {
            break;
        }
        node = node->prev();
    }
}


template<typename T>
template<class F>
void LinkList<T>::node_tail_do(F f) {
    auto node = this->_tail;
    while (node != nullptr) {
        if (!f(node)) {
            break;
        }
        node = node->prev();
    }
}

template<typename T>
bool LinkList<T>::verify() const {
    auto cur = this->_head;
    while (cur != nullptr) {
        const auto prev = cur->prev();
        const auto next = cur->next();
        if (prev != nullptr) {
            if (prev->next() != cur) {
                return false;
            }
        } else {
            if (prev != this->_head) {
                return false;
            }
        }
        if (next != nullptr) {
            if (next->prev() != cur) {
                return false;
            }
        } else {
            if (next != this->_tail) {
                return false;
            }
        }
        cur = cur->next();
    }
    return true;
}

template<typename T>
void LinkList<T>::head_add_to_list(T *node) {
    node->set_prev(nullptr);
    node->set_next(this->_head);
    if (this->_head != nullptr) {
        this->_head->set_prev(node);
    }
    this->_head = node;
    if (this->_tail == nullptr) {
        this->_tail = node;
    }
}

template<typename T>
void LinkList<T>::tail_add_to_list(T *node) {
    node->set_prev(this->_tail);
    node->set_next(nullptr);
    if (this->_tail != nullptr) {
        this->_tail->set_next(node);
    }
    this->_tail = node;
    if (this->_head == nullptr) {
        this->_head = node;
    }
}

template<typename T>
void LinkList<T>::delete_from_list(T *node) {
    assert(this->contain(node), "node is not be contained this list");
    auto prev = node->prev();
    auto next = node->next();
    if (prev != nullptr) {
        prev->set_next(next);
    } else {
        this->_head = next;
    }
    if (next != nullptr) {
        next->set_prev(prev);
    } else {
        this->_tail = prev;
    }

    node->set_next(nullptr);
    node->set_prev(nullptr);
}

template<typename T>
T *LinkList<T>::delete_from_list_head() {
    auto head = this->_head;
    if (head != nullptr) {
        this->delete_from_list(head);
    }
    return head;
}

template<typename T>
T *LinkList<T>::delete_from_list_tail() {
    auto tail = this->_tail;
    if (tail != nullptr) {
        this->delete_from_list(tail);
    }
    return tail;
}

template<typename T>
void LinkList<T>::add_to_list_target(T *target, T *node, bool before) {
    if (before && (target == nullptr || target->prev() == nullptr)) {
        //head insert
        this->head_add_to_list(node);
        return;
    }
    if (!before && (target == nullptr || target->next() == nullptr)) {
        //tail insert
        this->tail_add_to_list(node);
        return;
    }
    T *prev;
    T *next;
    if (before) {
        prev = target->prev();
        next = target;
    } else {
        prev = target;
        next = target->next();
    }
    prev->set_next(node);
    next->set_prev(node);
    node->set_prev(prev);
    node->set_next(next);
}

template<typename T>
bool LinkList<T>::contain(T *t) {
    assert(t != nullptr, "must be not null");
    auto cur = this->_head;
    while (cur != nullptr) {
        if (cur == t) {
            return true;
        }
        cur = cur->next();
    }
    return false;
}


#endif //NUCLEUSVM_LINKEDLIST_HPP
