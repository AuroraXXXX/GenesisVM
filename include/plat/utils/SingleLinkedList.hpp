//
// Created by aurora on 2024/6/26.
//

#ifndef PLATFORM_SINGLE_LINKED_LIST_HPP
#define PLATFORM_SINGLE_LINKED_LIST_HPP

#include "plat/utils/robust.hpp"

/**
 * 单链表
 * @tparam T 类型
 * next()
 * set_next()
 */
template<typename T>
class SingleLinkedList {
private:
    T *_head;
    T *_tail;
public:
    explicit SingleLinkedList() noexcept:
            _head(nullptr),
            _tail(nullptr) {};

    inline auto head() const {
        return this->_head;
    };

    inline auto tail() const {
        return this->_tail;
    };

    inline bool is_empty() const {
        return this->_head == nullptr;
    };

    void add_to_head(T *t) {
        assert(t != nullptr, "must be");
        t->set_next(this->_head);
        if (this->_tail == nullptr) {
            this->_tail = t;
        }
    }

    void add_to_tail(T *t) {
        assert(t != nullptr, "must be");
        t->set_next(this->_tail);
        if (this->_head == nullptr) {
            this->_head = t;
        }
    }
    /**
     * 遍历函数
     * @tparam F
     * @param func false 表示中止，不继续遍历
     */
    template<typename F>
    void iter(F func) {
        auto cur = this->_head;
        while (cur != nullptr){
            if(!func(cur)){
                break;
            }
            cur = cur ->next();
        }
    }
    /**
     *
     * @param F 返回值 true 表示是删除的节点
     */
    void remove(bool (*F)(T* node)){
        T* prev = nullptr;
        T* cur = this->_head;
        while (cur != nullptr){
            if(F(cur)){
               //说明要删除节点
               const auto next = cur->next();
               if(prev == nullptr){
                   //删除的是头节点
                   this->_head = next;
               }else{
                    prev->set_next(next);
               }
               cur->set_next(nullptr);
               if(next == nullptr){
                   this->_tail = cur;
               }
                break;
            }
            prev = cur;
            cur = cur->next();
        }
    }

};

#endif //PLATFORM_SINGLE_LINKED_LIST_HPP
