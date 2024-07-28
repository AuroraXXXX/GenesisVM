//
// Created by aurora on 2024/7/17.
//

#ifndef KERNEL_STILL_RUNNING_THREAD_CLOSURE_HPP
#define KERNEL_STILL_RUNNING_THREAD_CLOSURE_HPP

#include "kernel/thread/ThreadClosure.hpp"

class LangThread;

class StillRunningThreadClosure : public ThreadClosure {
private:
    friend class LangThread;

    LangThread *_still_list;
    size_t _still_num;
public:
    explicit StillRunningThreadClosure();

    void do_thread(PlatThread *thread) override;

    void clear();

    [[nodiscard]] inline auto still_num() const {
        return this->_still_num;
    };

    [[nodiscard]] inline auto still_list()const {
        return this->_still_list;
    };
};

#endif //KERNEL_STILL_RUNNING_THREAD_CLOSURE_HPP
