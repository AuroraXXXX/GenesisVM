//
// Created by aurora on 2024/2/25.
//

#include "VMThread.hpp"
#include "platform/concurrent/Monitor.hpp"
#include "platform/concurrent/safepoint.hpp"
#include "platform/log.hpp"
#include "platform/concurrent/VM_Operation.hpp"

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
    log_warn(nonuserthread)("%s,Exiting ...", this->name());
    //在安全点退出
    Safepoint::begin();
    {
        //通知其他线程 VMThread退出成功
        MonitorLocker ml(VMThreadTerminate_lock);
        this->_vm_state.store(VMState::terminated);
        log_warn(nonuserthread)("%s,Exited.", this->name());
        ml.notify();
    }
}

void VMThread::create() {
    //1 创建线程对象
    VMThread::_vm_thread = new VMThread();
    //2 创建实际的os对象
    if (!os::create_thread(VMThread::_vm_thread)) {
        vm_exit_during_initialization("Cannot create VM thread. "
                                      "Out of system resources.");
    } else {
        //
        log_info(nonuserthread)("%s:VMThread create is success!", VMThread::_vm_thread->name());
    }
}

void VMThread::destroy() {
    //由主线程调用
    const auto vm_thread = VMThread::vm_thread();
    {
        //1 首先获取 VMOperation_lock，确保没有VM操作需要被执行
        MonitorLocker ml(VMOperation_lock);
        //标记VMThread要进行终止，然后通知VMThread线程，要进行中止了
        vm_thread->_vm_state.store(VMState::should_terminate);
        ml.notify_all();
    }
    {
        //等待VMThread进行中止的相关操作。VMThrea执行完毕终止操作后，会自动唤醒本线程
        MonitorLocker ml(VMThreadTerminate_lock);
        while (vm_thread->_vm_state.load() != VMState::terminated) {
            ml.wait();
        }
    }
    VMThread::_vm_thread = nullptr;
}

VMThread::VMThread() :
        _cur_execute_operation(nullptr),
        _wait_execute_operation(nullptr),
        _vm_state(VMState::creating) {
}

void VMThread::loop() {
    assert(this->_cur_execute_operation == nullptr, "no current one should be executing");
    assert(VMThread::is_vm_thread_caller(), "Must be the VM thread");
    while (true) {
        //判断是不是要进行退出
        if (this->should_terminate())break;
        //等待操作
        {
            MonitorLocker mo_lock(VMThread::VMOperation_lock);
            //唤醒其他线程，说明已经
            mo_lock.notify_all();
            while (!this->should_terminate()) {
                if (this->_wait_execute_operation.load() != nullptr)
                    break;
                assert(this->_cur_execute_operation.load() == nullptr, "must be");
                assert(this->_wait_execute_operation.load() == nullptr, "must be");
                //没有什么可执行的 那么就需要再次唤醒可能等待此的线程
                mo_lock.notify_all();
                //自己再次无限期等待
                mo_lock.wait();
            }
        }
        //判断是不是要进行退出
        if (this->should_terminate())break;
        const auto wait_operation = this->_wait_execute_operation.load();
        assert(wait_operation != nullptr, "must have one");
        this->inner_execute(wait_operation);
        this->_wait_execute_operation.store(nullptr);
    }
}


void VMThread::inner_execute(VM_Operation *operation) {
    assert(VMThread::is_vm_thread_caller(), "Must be the VM thread");
    VM_Operation *prev_operation = nullptr;
    if (this->_cur_execute_operation.load() != nullptr) {
        //说明当前已经有正在执行的operation
        auto cur_execute_operation = this->_cur_execute_operation.load();
        guarantee(cur_execute_operation->allow_nested_vm_operations(),
                  "Unexpected nested VM operation %s requested by operation %s",
                  operation->name(), cur_execute_operation->name());
        //此处说明支持可重入调用，设置调用的线程应该是，
        operation->set_calling_thread(cur_execute_operation->calling_thread());
        //缓存 之前的操作
        prev_operation = cur_execute_operation;
    }

    // 将目前需要执行的operation 存储的 全局对象上
    this->_cur_execute_operation.store(operation);
    log_debug(nonuserthread)("%s:Evaluating %s %s VM operation: %s",
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
    //执行具体的函数
    operation->evaluate();
    //如果需要退出安全点，则执行推出安全点
    if (end_safepoint) {
        Safepoint::end();
    }
    //恢复 原本缓存的 要执行的operation
    this->_cur_execute_operation = prev_operation;
}

void VMThread::execute(VM_Operation *operation) {
    auto current = OSThread::current();
    operation->set_calling_thread(current);
    if (VMThread::is_vm_thread_caller()) {
        //说明是VMThread 无需进行等待
        ((VMThread *) current)->inner_execute(operation);
        return;
    }
    //执行前缀的处理，判断要不要执行 operation
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

        log_trace(nonuserthread)("UserThread(%d):Installing VM operation,cur timestamp:" SIZE_FORMAT,
                          os::current_thread_id(),
                          os::current_stamp());
        while (true) {
            //将待执行的operation 放入到VMThread中
            if (this->set_wait_operation(operation)) {
                //放入成功了 通知VMThread现在可以去执行了
                ml.notify_all();
                break;
            }
            //此处的顺序绝对不可以进行调整
            std::atomic_thread_fence(std::memory_order::seq_cst);
            //调用者本身 需要睡眠在这个在此处 由于
            log_trace(nonuserthread)("UserThread(%d):A VM operation already set, waiting", os::current_thread_id());
            // 说明没有 放入成功，有其他线程的operation待执行。本线程就睡眠在此处，等待其他线程待执行的operation执行完毕，重新放入
            ml.wait();
        }
    }
    //到这里说明 _next_operation一定是operation（即本operation已经放入到VMThread待执行中），所以本线程需要继续等待VMThread执行完毕
    {
        log_trace(nonuserthread)("UserThread(%d):Waiting for VM operation to be completed", os::current_thread_id());
        // 等待其被执行完毕
        while (this->_wait_execute_operation.load() == operation) {
            //被唤醒了，说明线程执行完毕了
            ml.wait();
        }
    }
}

bool VMThread::set_wait_operation(VM_Operation *operation) {
    if (this->_wait_execute_operation.load() != nullptr) {
        return false;
    }
    this->_wait_execute_operation.store(operation);
    log_debug(nonuserthread)("UserThread(%d):Adding VM operation: %s", os::current_thread_id(),
                      this->_wait_execute_operation.load()->name());
    assert(this->_wait_execute_operation.load() != nullptr, "must be");
    return true;
}

