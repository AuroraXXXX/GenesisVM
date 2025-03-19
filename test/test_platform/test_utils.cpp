//
// Created by aurora on 2025/3/18.
//
#include <iostream>
#include "platform/utils/LinkList.hpp"
class A:public LinkListNode<A>{
public:
    explicit A() = default;

};
int main(){
    LinkList<A> list;
    A a;
    list.push_head(&a);
    A b;
    list.push_head(&b);
    auto f =  list.pop_head();
    list.pop_head();
    auto result = list.verify();
    std::cout<<"hello:"<<result<<list.contain(&a)<<std::endl;

    return 0;
}