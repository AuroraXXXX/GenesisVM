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
    /**
     * 链表的首节点
     */
    T *_head;
    /**
     * 链表的尾节点
     */
    T *_tail;
    /**
     * 将节点从链表中删除
     * @param prev 前驱节点
     * @param cur 需要被删除的节点
     * @param total_after 需要删除节点的之后的所有节点 是不是也从链表上删除
     */
    void unlink(T* prev,T* cur,bool total_after){
        T* next = cur->next();
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

public:
    explicit SingleLinkedList() noexcept:
            _head(nullptr),
            _tail(nullptr) {};

    inline T* head() const {
        return this->_head;
    };

    inline T* tail() const {
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
    /**
     * 将节点添加到链表的尾部
     * @param t 节点
     */
    void add_to_head(T *t) {
        assert(t != nullptr, "must be");
        t->set_next(this->_head);
        if (this->_tail == nullptr) {
            this->_tail = t;
        }
    }
    /**
     * 将节点添加到链表头部
     * @param t 节点
     */
    void add_to_tail(T *t) {
        assert(t != nullptr, "must be");
        t->set_next(this->_tail);
        if (this->_head == nullptr) {
            this->_head = t;
        }
    }
    /**
     * 遍历函数，注意可 执行内存释放函数
     * @tparam F bool (T* node,size_t index) node表示当前遍历的节点 index表示当前的序号 ，返回值为false 终止执行
     * @param start 遍历的首节点
     * @param func 回调函数，内部可以执行当前节点的内存释放函数
     */
    template<typename F>
    static void iter(T* start,F func) {
        T* next;
        size_t index = 0;
        while (start != nullptr){
            next = start->next();
            if(!func(start,index)){
                break;
            }
            ++index;
            start = next;
        }
    };

    template<typename F>
    void iter(F func){
        SingleLinkedList<T>::iter(this->_head,func);
    };
    /**
     * 寻找要删除的节点的前驱节点
     * @param equal_func 寻找的删除节点的函数，true表示寻找到 需要进行删除
     * @param total_after 需要删除节点之后的节点也被从链表中删除
     */
    T*  remove(bool (*equal_func)(T* node),bool total_after = false){
        T* prev = nullptr;
        T* cur = this->_head;
        while (cur != nullptr){
            if (equal_func(cur)){
                //需要进行删除
                this->unlink(prev,cur,total_after);
                return cur;
            }
            prev = cur;
            cur = cur->next();
        }
        return nullptr;
    }
    /**
     * 寻找要删除的节点的前驱节点
     * @param equal_func 寻找的删除节点的函数，true表示寻找到 需要进行删除
     * @param total_after 需要删除节点之后的节点也被从链表中删除
     */
    T*  remove(T* node,bool total_after = false){
        T* prev = nullptr;
        T* cur = this->_head;
        while (cur != nullptr){
            if (cur == node){
                //需要进行删除
                this->unlink(prev,cur,total_after);
                return cur;
            }
            prev = cur;
            cur = cur->next();
        }
        return nullptr;
    }
    /**
     * 清空链表
     */
    void clear(){
        this->_head = nullptr;
        this->_tail = nullptr;
    }


};

#endif //PLATFORM_SINGLE_LINKED_LIST_HPP
