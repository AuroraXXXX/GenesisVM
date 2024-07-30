//
// Created by aurora on 2024/2/25.
//

#include "VMThread.hpp"
#include "kernel_mutex.hpp"
#include "kernel/utils/log.hpp"
#include "safepoint.hpp"

VMThread *VMThread::_vm_thread = nullptr;




void VMThread::run() {
    this->loop();
    //在安全点退出
    SafepointSynchronize::begin();
    {
        //通知其他线程 VMThread退出成功
        MonitorLocker ml(this->_terminate_lock);
        this->_is_terminate = true;
        ml.notify();
    }
}

void VMThread::create() {
    const auto thread = new VMThread();
    if(os::create_thread(thread)){
        VMThread::_vm_thread = thread;
    } else{
        guarantee(false,"init failed");
    }
}

void VMThread::destroy() {
    //由主线程调用
    {
        //通知VMThread线程，要进行中止了
        MonitorLocker mu(VMOperation_lock);
        VMThread::vm_thread()->_should_terminate = true;
        mu.notify_all();
    }
    {
        //等待VMThread进行中止
        auto terminate_lock = VMThread::vm_thread()->_terminate_lock;
        MonitorLocker ml(terminate_lock);
        while (!VMThread::vm_thread()->is_terminate()){
            ml.wait();
        }
    }
    VMThread::_vm_thread = nullptr;
}

VMThread::VMThread() :
        _cur_operation(nullptr),
        _next_operation(nullptr),
        _should_terminate(false),
        _is_terminate(false),
        _terminate_lock(nullptr) {
    _terminate_lock = new Monitor("VMThreadTerminate_lock");
}

void VMThread::loop() {
    assert(this->_cur_operation == nullptr, "no current one should be executing");
    while (true) {
        if (this->should_terminate())break;
        this->wait_for_operation();
        if (this->should_terminate())break;
        assert(_next_operation != nullptr, "must have one");
        this->inner_execute(_next_operation);
    }
}

void VMThread::wait_for_operation() {
    assert(PlatThread::current()->is_VM_thread(), "Must be the VM thread");
    MonitorLocker mo_lock(VMOperation_lock);
    //到这里说明等待的Operation也处理完毕了
    this->_next_operation = nullptr;
    //唤醒其他线程，说明已经
    mo_lock.notify_all();
    while (!this->_should_terminate) {
        if (OrderAccess::load(&this->_next_operation) != nullptr)
            return;
        assert(this->_next_operation == nullptr, "must be");
        assert(this->_cur_operation == nullptr, "must be");
        //没有什么可执行的 那么就需要再次唤醒可能等待此的线程
        mo_lock.notify_all();
        //自己再次无限期等待
        mo_lock.wait();
    }
}

void VMThread::inner_execute(VM_Operation *operation) {
    assert(PlatThread::current()->is_VM_thread(), "Must be the VM thread");
    VM_Operation *prev_operation = nullptr;
    if (this->_cur_operation != nullptr) {
        //说明当前已经有正在执行的operation
        guarantee(this->_cur_operation->allow_nested_vm_operations(),
                  "Unexpected nested VM operation %s requested by operation %s",
                  operation->name(), this->_cur_operation->name());
        operation->set_calling_thread(this->_cur_operation->calling_thread());
        prev_operation = this->_cur_operation;
    }
    // 表示当前执行的线程
    this->_cur_operation = operation;
    log_debug(vmthread)("Evaluating %s %s VM operation: %s",
                        prev_operation != nullptr ? "nested" : "",
                        _cur_operation->evaluate_at_safepoint() ? "safepoint" : "non-safepoint",
                        _cur_operation->name());
    //是否在安全点结束
    bool end_safepoint = false;
    if (this->_cur_operation->evaluate_at_safepoint() &&
        !SafepointSynchronize::is_at_safepoint()) {
        // VM_Operation要求在安全点执行，并且在现在没有在安全点
        SafepointSynchronize::begin();
        end_safepoint = true;
    }
    this->_cur_operation->evaluate();
    if (end_safepoint) {
        SafepointSynchronize::end();
    }
    this->_cur_operation = prev_operation;
}

void VMThread::execute(VM_Operation *operation) {
    auto current = PlatThread::current();
    if (current->is_VM_thread()) {
        //说明是VMThread 无需进行等待
        operation->set_calling_thread(current);
        ((VMThread *) current)->inner_execute(operation);
        return;
    }
    if (!operation->doit_prologue()) {
        return;
    }
    //非Thread线程就开始等到operation被执行完毕
    VMThread::wait_until_executed(operation);
    //执行尾处理
    operation->doit_epilogue();
}

void VMThread::wait_until_executed(VM_Operation *operation) {
    MonitorLocker ml(VMOperation_lock);
    {
        log_trace(vmthread)("Installing VM operation,cur timestamp:" SIZE_FORMAT,
                            os::current_stamp());
        while (true) {
            if (VMThread::vm_thread()->set_next_operation(operation)) {
                //放入成功了 通知VMThread现在可以去执行了
                ml.notify_all();
                break;
            }
            log_trace(vmthread)("A VM operation already set, waiting");
            ml.wait();
        }
    }
    //到这里说明 _next_operation一定是operation
    {
        log_trace(vmthread)("Waiting for VM operation to be completed");
        // 等待其被执行完毕
        while (VMThread::vm_thread()->_next_operation == operation) {
            ml.wait();
        }
    }
}

bool VMThread::set_next_operation(VM_Operation *operation) {
    if (this->_next_operation != nullptr) {
        return false;
    }
    log_debug(vmthread)("Adding VM operation: %s", operation->name());

    this->_next_operation = operation;
    return true;
}


