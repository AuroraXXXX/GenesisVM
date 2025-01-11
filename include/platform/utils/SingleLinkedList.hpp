//
// Created by aurora on 2024/6/26.
//

#ifndef PLATFORM_SINGLE_LINKED_LIST_HPP
#define PLATFORM_SINGLE_LINKED_LIST_HPP

#include "platform/utils/robust.hpp"

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

    void unlink(T* prev,T* cur){
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
    }
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
     * @param func false 表示中止，不继续遍历 内部可以执行删除操作
     */
    template<typename F>
    void iter(F func) {
        auto cur = this->_head;
        T* next;
        while (cur != nullptr){
            next = cur->next();
            if(!func(cur)){
                break;
            }
            cur = next;
        }
    }
    /**
     * 删除链表
     * @param F 返回值 true 表示是删除的节点
     * @param many_after true 表示包括当前节点和之后的所有节点
     */
    void remove(bool (*F)(T* node),bool many_after = false){
        T* prev = nullptr;
        T* cur = this->_head;
        while (cur != nullptr){
            if(F(cur)){
               //说明要删除节点
                if (many_after){
                    //删除多个
                    T* next;
                    do {
                        next = cur->next();
                        this->unlink(prev,cur);
                        cur = next;
                    } while (cur != nullptr);
                } else{
                    //仅仅删除一个
                    this->unlink(prev,cur);
                }
                break;
            }
            prev = cur;
            cur = cur->next();
        }
    }
    /**
     * 清楚链表
     */
    void clear(){
        this->_head = nullptr;
        this->_tail = nullptr;
    }


};

#endif //PLATFORM_SINGLE_LINKED_LIST_HPP
