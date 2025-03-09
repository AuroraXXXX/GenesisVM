//
// Created by aurora on 2024/2/25.
//

#ifndef KERNEL_THREAD_VM_THREAD_HPP
#define KERNEL_THREAD_VM_THREAD_HPP

#include "platform/concurrent/OSThread.hpp"
#include "daemon-thread/VM_Operation.hpp"
#include <atomic>

class Monitor;


/**
 * 提供全局的安全点
 */
class VMThread : public DaemonThread {
public:
    enum class VMState {
        creating,
        //正在执行
        running,
        //应该被终止，会逐步终止
        should_terminate,
        //已经进入到终止状态
        terminated
    };
private:

    /**
     * 存储 具体的vm thread实例
     */
    static VMThread *_vm_thread;
    /**
     * 运行VMOperation所需的lock
     */
    static Monitor *VMOperation_lock;
    /**
     * 锁定,当线程释放时。会唤醒等待在该锁上的线程
     */
    static Monitor *VMThreadTerminate_lock;
    /**
     * 标记线程是否应该终止
     */
    std::atomic<VMState> _vm_state;
    /**
     * 当前正在执行的 operation
     */
    std::atomic<VM_Operation *> _cur_execute_operation;
    /**
     * 等待执行的 operation
     */
    std::atomic<VM_Operation *> _wait_execute_operation;

    inline auto should_terminate() {
        return this->_vm_state.load() == VMState::should_terminate;
    }

    explicit VMThread();

    /**
     * 实际循环执行operation
     */
    void loop();


    /**
     * VM_Thread本身去执行VM_Operation
     * @param operation
     */
    void inner_execute(VM_Operation *operation);

    /**
     * 非VMThread等待VM_Operation被VMThread执行完毕
     * @param operation
     */
    void wait_until_executed(VM_Operation *operation);

    /**
     * 将VM_Operation放入到队列上,等待被执行
     * @param operation
     * @return 操作是否成功
     */
    bool set_wait_operation(VM_Operation *operation);


public:

    static inline VMThread *vm_thread() {
        return _vm_thread;
    };

    /**
     * 创建内核线程
     */
    static void create();

    static void destroy();

    /**
     * 向VMThread发送一个VM_Operation，并等待其完成
     * @param operation
     */
    static void execute(VM_Operation *operation);

    const char *name() override {
        return "VMThread";
    };

    /**
     * 判断调用者所在的线程对象是
     * @return
     */
    static bool is_vm_thread_caller() {
        return VMThread::_vm_thread == OSThread::current();
    };
protected:
    void run() override;
};


#endif //KERNEL_THREAD_VM_THREAD_HPP
