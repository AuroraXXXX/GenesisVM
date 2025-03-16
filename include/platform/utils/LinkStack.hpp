//
// Created by aurora on 2025/3/16.
//

#ifndef PLATFORM_LINK_STACK_HPP
#define PLATFORM_LINK_STACK_HPP

#include "platform/typedef.hpp"

/**
 * 使用单链表形成的堆栈
 */
template<typename T>
class LinkStack {
private:
    T *_top;
    size_t _num;
public:
    explicit LinkStack() : _top(nullptr), _num(0) {};


    ~LinkStack() {
        // 调用clear()函数，清空栈
        this->clear();
    };

    /**
     *  清空栈
     */
    void clear() {
        // 将栈顶指针置为空
        this->_top = nullptr;
        // 将栈中元素数量置为0
        this->_num = 0;
    }

    /**
     * 将节点t压入到栈中
     * @param t
     */
    void push(T *t) {
        //
        if (t == nullptr) return;
        t->set_next(this->_top);
        this->_top = t;
        this->_num++;
    };

    /**
     * 弹出栈顶元素
     * @return
     */
    T *pop() {
        if (this->_top == nullptr) return nullptr;
        T *t = this->_top;
        this->_top = t->next();
        this->_num--;
        t->set_next(nullptr);
        return t;
    };

    /**
     * 返回栈顶元素,但是并不会修改栈中数据
     * @return
     */
    T *peek() {
        return this->_top;
    };

    /**
     * 获取栈中元素数量
     * @return
     */
    [[nodiscard]] inline bool get_num() const {
        return this->_num;
    };

    [[nodiscard]] inline bool is_empty() const {
        return this->_num == 0;
    };

    /**
     * 遍历堆栈，不能执行删除函数，调整链表中节点的顺序
     * @tparam F void f(T *t,size_t index)
     *          t:当前节点
     *          index:当前节点在栈中的位置
     * @param f 具体的回调函数
     */
    template<typename F>
    void iterate(F f) {
        size_t index = 0;
        for (T *t = this->_top; t != nullptr; t = t->next()) {
            f(t, index++);
        }
    };
    /**
     * 在堆栈中删除指定节点t
     * @param t 要被删除节点
     * @return 操作是否成功
     */
    bool remove(T *t) {
        if (t == nullptr) return false;
        T* prev = nullptr;
        T* cur = this->_top;
        while (cur != nullptr) {
            if (cur == t) {
                if (prev == nullptr) {
                    this->_top = cur->next();
                } else {
                    prev->set_next(cur->next());
                }
                this->_num--;
                //清楚当前所属关系
                t->set_next(nullptr);
                return true;
            }
            prev = cur;
            cur = cur->next();
        }
        return false;
    }
};

#endif //PLATFORM_LINK_STACK_HPP
