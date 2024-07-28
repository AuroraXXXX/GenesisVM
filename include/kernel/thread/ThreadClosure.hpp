//
// Created by aurora on 2024/7/14.
//

#ifndef KERNEL_THREAD_CLOSURE_HPP
#define KERNEL_THREAD_CLOSURE_HPP

#include "kernel/utils/closure.hpp"

class PlatThread;

class ThreadClosure : public Closure {

public:
    /**
     * @param thread
     */
    virtual void do_thread(PlatThread *thread) {};

};

#endif //KERNEL_THREAD_CLOSURE_HPP
