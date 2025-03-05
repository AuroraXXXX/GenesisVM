//
// Created by aurora on 2024/2/25.
//

#include "VMThread.hpp"
#include "platform/concurrent/Monitor.hpp"
#include "platform/concurrent/safepoint.hpp"
#include "platform/log.hpp"
#include "daemon-thread/VM_Operation.hpp"

VMThread *VMThread::_vm_thread = nullptr;
/**
 * 运行VMOperation所需的lock
 */
Monitor *VMThread::VMOperation_lock = new Monitor("VMOperation_lock");
/**
 * 锁定,当线程释放时。会唤醒等待在该锁上的线程
 */
Monitor *VMThread::VMThreadTerminate_lock = new Monitor("VMThreadTerminate_lock");

void VMThread::run() {
    this->loop();
    //在安全点退出
    Safepoint::begin();
    {
        //通知其他线程 VMThread退出成功
        MonitorLocker ml(VMThreadTerminate_lock);

        this->_is_terminate.store(true);
        ml.notify();
    }
}

void VMThread::create() {
    const auto thread = new VMThread();
    if (os::create_thread(thread)) {
        VMThread::_vm_thread = thread;
    } else {
        guarantee(false, "init failed");
    }
}

void VMThread::destroy() {
    //由主线程调用
    const auto vm_thread = VMThread::vm_thread();
    {
        //1 首先获取 VMOperation_lock，确保没有VM操作需要被执行
        MonitorLocker mu(VMOperation_lock);
        //通知VMThread线程，要进行中止了
        vm_thread->_should_terminate.store(true);
        mu.notify_all();
    }
    {
        //等待VMThread进行中止
        //auto terminate_lock = VMThread::vm_thread()->_terminate_lock;
        MonitorLocker ml(VMOperation_lock);
        while (!vm_thread->_is_terminate.load()) {
            ml.wait();
        }
    }
    VMThread::_vm_thread = nullptr;
}

VMThread::VMThread() :
        _cur_operation(nullptr),
        _next_operation(nullptr),
        _should_terminate(false),
        _is_terminate(false) {
}

void VMThread::loop() {
    assert(this->_cur_operation == nullptr, "no current one should be executing");
    while (true) {
        if (this->_should_terminate.load())break;
        this->wait_for_operation();
        if (this->_should_terminate.load())break;
        const auto next_operation = this->_next_operation.load();
        assert(next_operation != nullptr, "must have one");
        this->inner_execute(next_operation);
    }
}

void VMThread::wait_for_operation() {
    assert(VMThread::is_vm_thread_caller(), "Must be the VM thread");
    MonitorLocker mo_lock(VMThread::VMOperation_lock);
    //到这里说明等待的Operation也处理完毕了
    this->_next_operation.store(nullptr);
    //OrderAccess::fence();
    //唤醒其他线程，说明已经
    mo_lock.notify_all();
    while (!this->_should_terminate.load()) {
        if (this->_next_operation.load() != nullptr)
            return;
        assert(this->_next_operation.load() == nullptr, "must be");
        assert(this->_cur_operation.load() == nullptr, "must be");
        //没有什么可执行的 那么就需要再次唤醒可能等待此的线程
        mo_lock.notify_all();
        //自己再次无限期等待
        mo_lock.wait();
    }
}

void VMThread::inner_execute(VM_Operation *operation) {
    assert(VMThread::is_vm_thread_caller(), "Must be the VM thread");
    VM_Operation *prev_operation = nullptr;
    if (this->_cur_operation != nullptr) {
        //说明当前已经有正在执行的operation
        guarantee(this->_cur_operation.load()->allow_nested_vm_operations(),
                  "Unexpected nested VM operation %s requested by operation %s",
                  operation->name(), this->_cur_operation.load()->name());
        operation->set_calling_thread(this->_cur_operation.load()->calling_thread());
        prev_operation = this->_cur_operation;
    }
    // 表示当前执行的线程
    this->_cur_operation.store(operation);
    log_debug(daemon)("%s:Evaluating %s %s VM operation: %s",
                      this->name(),
                      prev_operation != nullptr ? "nested" : "",
                      operation->evaluate_at_safepoint() ? "safepoint" : "non-safepoint",
                      operation->name());
    //是否在安全点结束
    bool end_safepoint = false;
    if (operation->evaluate_at_safepoint() &&
        !Safepoint::is_at_safepoint()) {
        // VM_Operation要求在安全点执行，并且在现在没有在安全点
        Safepoint::begin();
        end_safepoint = true;
    }
    operation->evaluate();
    if (end_safepoint) {
        Safepoint::end();
    }
    this->_cur_operation = prev_operation;
}

void VMThread::execute(VM_Operation *operation) {
    auto current = OSThread::current();
    operation->set_calling_thread(current);
    if (VMThread::is_vm_thread_caller()) {
        //说明是VMThread 无需进行等待
        ((VMThread *) current)->inner_execute(operation);
        return;
    }
    if (!operation->doit_prologue()) {
        return;
    }
    //非Thread线程就开始等到operation被执行完毕
    VMThread::vm_thread()->wait_until_executed(operation);
    //执行尾处理
    operation->doit_epilogue();
}

void VMThread::wait_until_executed(VM_Operation *operation) {
    MonitorLocker ml(VMThread::VMOperation_lock);
    {

        log_trace(daemon)("%s:Installing VM operation,cur timestamp:" SIZE_FORMAT,
                          this->name(),
                          os::current_stamp());
        while (true) {
            if (this->set_next_operation(operation)) {
                //放入成功了 通知VMThread现在可以去执行了
                ml.notify_all();
                break;
            }
            //OrderAccess::fence();
            log_trace(daemon)("%s:A VM operation already set, waiting", this->name());
            ml.wait();
        }
    }
    //到这里说明 _next_operation一定是operation
    {
        log_trace(daemon)("%s:Waiting for VM operation to be completed", this->name());
        // 等待其被执行完毕
        while (this->_next_operation.load() == operation) {
            ml.wait();
        }
    }
}

bool VMThread::set_next_operation(VM_Operation *operation) {
    if (this->_next_operation.load() != nullptr) {
        return false;
    }
    this->_next_operation.store(operation);
    log_debug(daemon)("%s:Adding VM operation: %s", this->name(), this->_next_operation.load()->name());
    assert(this->_next_operation.load() != nullptr, "must be");
    return true;
}

