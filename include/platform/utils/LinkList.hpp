//
// Created by aurora on 2024/1/31.
//

#ifndef PLATFORM_LINK_LIST_HPP
#define PLATFORM_LINK_LIST_HPP

#include <type_traits>
#include "platform/utils/robust.hpp"

/**
 * 链表节点
 */
template<typename T>
class LinkListNode {
private:
    T *_prev;
    T *_next;
public:
    explicit LinkListNode() : _prev(nullptr), _next(nullptr) {}

    inline void set_prev(T *prev) {
        _prev = prev;
    }

    inline void set_next(T *next) {
        _next = next;
    }

    inline T *prev() {
        return _prev;
    }

    inline T *next() {
        return _next;
    }

    bool is_clear() {
        return this->_next == nullptr && this->_prev == nullptr;
    };

    void clear(){
        this->_prev = nullptr;
        this->_next = nullptr;
    };
};

template<typename T>
using GetLinkListNodeFuncType = LinkListNode<T> *(T::*)();

/**
 * 概念介绍：
 * 1. T 表示是一个存储类型
 * 2. LinkListNode 表示的链表节点
 * T 与 LinkListNode 关系是 1 对 多的关系。
 * 即 T 可以继承 LinkListNode，那么T与 LinkListNode 是1 对 1 的关系，此时 GetLinkListNodeFunc 指定为null即可。
 * 若 T 类型可以存储在多个链表中，那么T与 LinkListNode 是1 对 多 的关系,此时 GetLinkListNodeFunc 指定为 返回本链表所使用的节点地址的T的成员函数
 * @tparam T 存储类型
 * @tparam GetLinkListNodeFunc 获取链表节点的类成员函数
 */
template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc = nullptr>
class LinkList {
private:
    /**
     * _head _tail
     */
    T *_head;
    T *_tail;

    /**
     *
     * @param t 存储类型的一个实例
     * @return 返回其使用的 LinkListNode
     */
    static inline LinkListNode<T> *get_adjacent_node(T *t) {
        assert(t != nullptr, "must be not null");
        LinkListNode<T> *node;
        if constexpr (GetLinkListNodeFunc != nullptr) {
            // 指定了获取节点的类成员函数，我们调用类成员函数，获取节点的地址
            node = (t->*GetLinkListNodeFunc)();
        } else {
            static_assert(GetLinkListNodeFunc != nullptr || std::is_base_of_v<LinkListNode<T>, T>,
                          "当 GetLinkListNodeFunc 为 nullptr 时，T 必须继承自 LinkListNode<T>");
            // 没有指定获取节点的类成员函数，说明存储类型本身就是可以转换成节点地址
            node = t;
        }
        assert(node != nullptr, "must be not null");
        return node;
    };

public:
    explicit LinkList() : _head(nullptr), _tail(nullptr) {};

    /**
     * 从头部开始向后遍历
     * @tparam F bool func(T* cur); 返回值表示是否继续遍历下一个
     * @param func 函数实例
     */
    template<typename F>
    void head_do(F func);

    /**
     *  从尾节点节点向前进行遍历
     * @tparam F bool func(T* cur); 返回值表示是否继续遍历下一个
     * @param func 函数实例
     */
    template<typename F>
    void tail_do(F func);

    /**
     * 将t放入链表的头部
     * @param t 存储类型的实例
     */
    void push_head(T *t);

    /**
     * 将t放入链表的尾部
     * @param t 存储类型的实例
     */
    void push_tail(T *t);

    /**
     *  从链表头部取出一个元素
     * @return
     */
    T *pop_head();

    /**
     * 从链表尾部取出一个元素
     * @return
     */
    T *pop_tail();

    /**
     * 判断链表是否为空
     * @return
     */
    inline bool is_empty() {
        return this->_head == nullptr;
    };

    /**
     * 判断链表中是否包含t
     * @param t 存储类型的实例
     * @return
     */
    bool contain(T *t);

    /**
     * 校验整个链表
     * @return
     */
    [[nodiscard]] bool verify() const;

    /**
     * 将节点t插入到target附近，具体target的之前还是之后，由 is_prev 控制
     * @param t
     * @param target
     * @param is_prev true 插入到target之前； false 表示之后
     */
    void add(T *t, T *target, bool is_prev);

    /**
     * 删除 节点t
     * @param t
     */
    void remove(T *t);
};

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
void LinkList<T, GetLinkListNodeFunc>::remove(T *t) {
    if (t == nullptr) {
        return;
    }
    //进行实际的删除操作
    auto node = LinkList::get_adjacent_node(t);
    T *prev = node->prev();
    T *next = node->next();
    if (prev != nullptr) {
        // 说明不是 第一个节点
        LinkList::get_adjacent_node(prev)->set_next(next);
    } else {
        //第一个直接修改节点
        this->_head = next;
    }
    if (next != nullptr) {
        LinkList::get_adjacent_node(next)->set_prev(prev);
    } else {
        this->_tail = prev;
    }
    //清空取下的节点的 前驱和后继节点信息 防止污染
    node->clear();
}

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
void LinkList<T, GetLinkListNodeFunc>::add(T *t, T *target, bool is_prev) {
    if (t == nullptr || target == nullptr) {
        return;
    }
    //获取前驱 和 后继 存储节点
    auto node = LinkList::get_adjacent_node(t);
    auto target_node = LinkList::get_adjacent_node(target);
    if (is_prev) {
        //插入到target之前
        T *prev = target_node->prev();
        if (prev) {
            //说明不是第一个节点
            LinkList::get_adjacent_node(prev)->set_next(t);
        } else {
            //说明是第一个节点
            this->_head = t;
        }
        target_node->set_prev(t);
        //设置node节点的前后的信息
        node->set_prev(prev);
        node->set_next(target);
    } else {
        //插入到target之后
        T *next = target_node->next();
        if (next) {
            //说明不是最后一个节点
            LinkList::get_adjacent_node(next)->set_prev(t);
        } else {
            //说明是最后一个节点
            this->_tail = t;
        }
        target_node->set_next(t);
        //设置node节点的前后的信息
        node->set_prev(target);
        node->set_next(next);
    }

}

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
bool LinkList<T, GetLinkListNodeFunc>::contain(T *t) {
    if (t == nullptr) {
        return false;
    }
    bool result;
    const auto lambda_func = [&](T *cur) -> bool {
        bool equals = t == cur;
        result = equals;
        return !equals;
    };
    this->head_do(lambda_func);
    return result;
}

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
bool LinkList<T, GetLinkListNodeFunc>::verify() const {
    T *cur = this->_head;
    while (cur != nullptr) {
        //获取前驱 和 后继 存储节点
        LinkListNode<T> *cur_node = LinkList::get_adjacent_node(cur);
        const auto prev = cur_node->prev();
        const auto next = cur_node->next();
        if (prev != nullptr) {
            T *prev_next = LinkList::get_adjacent_node(prev)->next();
            if (prev_next != cur) {
                return false;
            }
        } else {
            //说明是头节点
            if (cur != this->_head) {
                return false;
            }
        }
        if (next != nullptr) {
            T *next_prev = LinkList::get_adjacent_node(next)->prev();
            if (next_prev != cur) {
                return false;
            }
        } else {
            //说明是尾节点
            if (cur != this->_tail) {
                return false;
            }
        }
        cur = next;
    }
    return true;
}


template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
T *LinkList<T, GetLinkListNodeFunc>::pop_head() {
    if (this->_head == nullptr) {
        // 队列中空的
        return nullptr;
    }
    // 要返回的节点
    T *t = this->_head;

    // 获取对应的链表节点
    const auto node = LinkList::get_adjacent_node(t);
    auto next = node->next();
    if (next != nullptr) {
        //说明next 也存储了类型 ，那么也要将其前驱 清空
        auto next_node = LinkList::get_adjacent_node(next);
        next_node->set_prev(nullptr);
    }
    //设置 next
    this->_head = next;
    if (this->_head == nullptr) {
        this->_tail = nullptr;
    }
    node->clear();
    return t;
}


template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
void LinkList<T, GetLinkListNodeFunc>::push_head(T *t) {
    if (t == nullptr) {
        // 如果 节点是空的
        return;
    }
    auto link_node = LinkList::get_adjacent_node(t);
    //设置所使用的 链表节点的 前驱和后继关系
    link_node->set_prev(nullptr);
    link_node->set_next(this->_head);

    if (this->_head != nullptr) {
        //说明head存储了 类型，那么 我们需要设置其关联的 链表节点的
        link_node = LinkList::get_adjacent_node(this->_head);
        //因为 上面已经将 head节点放到 t之后了，那么 就需要设置 head前驱为t
        link_node->set_prev(t);
    }
    this->_head = t;
    if (this->_tail == nullptr) {
        this->_tail = t;
    }
}

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
void LinkList<T, GetLinkListNodeFunc>::push_tail(T *t) {
    if (t == nullptr) {
        // 如果 节点是空的
        return;
    }
    auto link_node = LinkList::get_adjacent_node(t);
    //设置所使用的 链表节点的 前驱和后继关系
    link_node->set_prev(this->_tail);
    link_node->set_next(nullptr);

    if (this->_tail != nullptr) {
        //说明tail存储了 类型，那么 我们需要设置其关联的 链表节点的
        link_node = LinkList::get_adjacent_node(this->_tail);
        //因为 上面已经
        link_node->set_next(t);
    }
    this->_tail = t;
    if (this->_head == nullptr) {
        this->_head = t;
    }
}

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
T *LinkList<T, GetLinkListNodeFunc>::pop_tail() {
    if (this->_tail == nullptr) {
        // 队列中空的
        return nullptr;
    }
    // 要返回的节点
    T *t = this->_tail;
    // 获取对应的链表节点
    const  auto node = LinkList::get_adjacent_node(t);
    auto prev = node->prev();
    if (prev != nullptr) {
        //说明prev 也存储了类型 ，那么也要将其后继 清空
        auto prev_node = LinkList::get_adjacent_node(prev);
        prev_node->set_next(nullptr);
    }
    //设置 prev
    this->_tail = prev;
    if (this->_tail == nullptr) {
        //说明 当前删除后，链表是空的
        this->_head = nullptr;
    }
    node->clear();
    return t;
}


template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
template<typename F>
void LinkList<T, GetLinkListNodeFunc>::head_do(F func) {
    T *cur = this->_head;
    while (cur != nullptr) {
        bool continue_next = func(cur);
        if (!continue_next) {
            break;
        }
        //获取 链表节点
        cur = LinkList::get_adjacent_node(cur)->next();
    }
}

template<typename T, GetLinkListNodeFuncType<T> GetLinkListNodeFunc>
template<typename F>
void LinkList<T, GetLinkListNodeFunc>::tail_do(F func) {
    T *cur = this->_tail;
    while (cur != nullptr) {
        bool continue_next = func(cur);
        if (!continue_next) {
            break;
        }
        //获取 链表节点
        cur = LinkList::get_adjacent_node(cur)->prev();
    }
}

#endif //PLATFORM_LINK_LIST_HPP
