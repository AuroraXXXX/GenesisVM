//
// Created by aurora on 2024/2/13.
//
#ifndef PLATFORM_THREAD_STATUS_TRANS_HPP
#define PLATFORM_THREAD_STATUS_TRANS_HPP


#include "platform/allocation.hpp"
class OSThread;
class ThreadStatusBlockedTrans : public StackObject {
private:
    OSThread *_self;
public:
    explicit ThreadStatusBlockedTrans() ;

    ~ThreadStatusBlockedTrans() ;

};

#endif //PLATFORM_THREAD_STATUS_TRANS_HPP
