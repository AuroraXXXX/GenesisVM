//
// Created by aurora on 2024/2/25.
//

#ifndef KERNEL_THREAD_VM_THREAD_HPP
#define KERNEL_THREAD_VM_THREAD_HPP

#include "kernel/thread/PlatThread.hpp"
#include "kernel/thread/VM_Operation.hpp"


class Monitor;


/**
 * 提供全局的安全点
 */
class VMThread : public PlatThread {
private:
    volatile bool _should_terminate;
    volatile bool _is_terminate;
    static VMThread *_vm_thread;
    VM_Operation *volatile _cur_operation;
    VM_Operation *volatile _next_operation;
    /**
     * 锁定
     */
    Monitor *const _terminate_lock;

    explicit VMThread();

    void loop();

    /**
     * VMThread本身等待VM_Operation去执行
     */
    void wait_for_operation();

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
    bool set_next_operation(VM_Operation *operation);

    [[nodiscard]] inline auto should_terminate() const {
        return OrderAccess::load(&this->_should_terminate);
    };

    [[nodiscard]] inline auto is_terminate() const {
        return OrderAccess::load(&this->_is_terminate);
    };

    [[nodiscard]] inline auto cur_operation() const {
        return OrderAccess::load(&this->_cur_operation);
    };

    [[nodiscard]] inline auto next_operation() const {
        return OrderAccess::load(&this->_next_operation);
    };
public:

    static inline VMThread *vm_thread() {
        return _vm_thread;
    };

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

    bool is_VM_thread() override {
        return true;
    };

    bool is_user_thread() override {
        return false;
    };

    bool is_daemon_thread() override {
        return true;
    };

protected:
    void run() override;
};


#endif //KERNEL_THREAD_VM_THREAD_HPP
