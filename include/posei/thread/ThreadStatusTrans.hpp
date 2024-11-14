//
// Created by aurora on 2024/2/13.
//

#ifndef PLAT_THREAD_STATUS_TRANS_HPP
#define PLAT_THREAD_STATUS_TRANS_HPP

#include "plat/mem/allocation.hpp"
class OSThread;
class ThreadStatusBlockedTrans : public StackObject {
private:
    OSThread *_self;
public:
    explicit ThreadStatusBlockedTrans() ;

    ~ThreadStatusBlockedTrans() ;

};

#endif //PLAT_THREAD_STATUS_TRANS_HPP
