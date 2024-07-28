//
// Created by aurora on 2024/2/26.
//

#ifndef KERNEL_THREAD_PLAT_THREAD_HPP
#define KERNEL_THREAD_PLAT_THREAD_HPP

#include "plat/thread/OSThread.hpp"
#include "kernel/thread/ThreadClosure.hpp"

/**
 * 代表一个平台
 */
class PlatThread : public OSThread {
private:
    PlatThread *volatile _next;
    static PlatThread *volatile _user_thread_list;
    static PlatThread *volatile _daemon_thread_list;

    void add_to_list();

    void remove_from_list();

protected:

    void pre_run() override;

    void post_run() override;

public:
    /**
     * 遍历的工具函数
     * @param list_ptr 数组的首地址的指针
     * @param closure 遍历的Closure
     */
    static void thread_do(PlatThread *volatile *list_ptr,
                          ThreadClosure *closure);
    /**
     * 迭代所有守护线程
     * @param closure
     */
    inline static void thread_do_daemon(ThreadClosure* closure){
        thread_do(&PlatThread::_daemon_thread_list,closure);
    }

    /**
     * 迭代所有用户线程
     * @param closure
     */
    inline static void thread_do_user(ThreadClosure* closure){
        thread_do(&PlatThread::_user_thread_list,closure);
    }
    template<typename T = PlatThread>
    static inline T *current() {
        assert(dynamic_cast<T *>(OSThread::current()) != nullptr, "cannot convert");
        return dynamic_cast<T *>(OSThread::current());
    };

    virtual bool is_user_thread() = 0;

    virtual bool is_daemon_thread() = 0;

    virtual bool is_VM_thread() {
        return false;
    };


    virtual bool is_gc_worker_thread() {
        return false;
    };

    virtual bool is_watcher_thread() {
        return false;
    };

    virtual const char *name() {
        return "PlatThread";
    };

    explicit PlatThread();

    ~PlatThread() override {
        assert(this->_next == nullptr, "is not be delete from list");
    };


};


#endif //KERNEL_THREAD_PLAT_THREAD_HPP
