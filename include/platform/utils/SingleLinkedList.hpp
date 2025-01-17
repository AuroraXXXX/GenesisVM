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
     inline void set_head(T* head){
         this->_head = head;
     };
    inline void set_tail(T* tail){
        this->_tail = tail;
    };
    [[nodiscard]] inline bool is_empty() const {
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
    static void iter(T* cur,F func) {
        T* next;
        size_t index = 0;
        while (cur != nullptr){
            next = cur->next();
            if(!func(cur,index)){
                break;
            }
            ++index;
            cur = next;
        }
    }
    /**
     * 寻找要删除的节点的前驱节点
     * @param F
     */
    void find_prev_node(bool (*equal_func)(T* node)){
        T* prev = nullptr;
        T* cur = this->_head;
        while (cur != nullptr){
            if (equal_func(cur)){
                break;
            }
            prev = cur;
            cur = cur->next();
        }
        return prev;
    }
    /**
     *
     * @param prev
     */
    void unlink(T* prev,bool total_after = false){
        T* cur;
        if(prev == nullptr){
            cur = this->_head;
        }else{
            cur = prev->next();
        }
        if(cur == nullptr){
            //说明当前的节点是空
            return;
        }
        const auto next = cur->next();
        if(prev == nullptr){
            //删除的是头节点
            this->_head = next;
        }else{
            prev->set_next(next);
        }
        //设置 结尾
        if (total_after){
            //之后全部都要进行删除
            this->_tail = prev;
        } else{
            //不是后面都要删除 ，只需要删除当前一个
            cur->set_next(nullptr);
            this->_tail = next;
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
