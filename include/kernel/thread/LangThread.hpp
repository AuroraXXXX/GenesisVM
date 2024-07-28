//
// Created by aurora on 2023/1/7.
//

#ifndef KERNEL_THREAD_LANG_THREAD_HPP
#define KERNEL_THREAD_LANG_THREAD_HPP


#include "kernel/thread/ThreadClosure.hpp"
#include "plat/utils/robust.hpp"
#include "kernel/thread/PlatThread.hpp"
/**
 * 用于支持语言层面的线程
 */
class LangThread : public PlatThread {
    //用于_still_running_next
    friend class StillRunningThreadClosure;
private:
    /**
     * VMThread在安全点检查仍在运行LangThread的时候使用
     * 即由safepoint进行调整
     * 详情 见 SafePointSynchronize::synchronize_threads函数
     */
    LangThread *_still_running_next;


protected:
    void state_transitioning_callback(uint8_t from_state, uint8_t to_state) override;

    void run() override{};

public:
    bool is_user_thread() override {
        return true;
    };

    bool is_daemon_thread() override {
        return false;
    };

    const char *name() override {
        return "LangThread";
    };

    explicit LangThread();
     ~LangThread() override = default;
};


#endif //KERNEL_THREAD_LANG_THREAD_HPP