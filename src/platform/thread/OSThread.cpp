//
// Created by aurora on 2022/12/5.
//

#include "platform/thread/OSThread.hpp"
#include "platform/utils/robust.hpp"
#include "platform/stream/CharOStream.hpp"
#include "platform/os.hpp"
#include <pthread.h>

thread_local OSThread *OSThread::_current = nullptr;
OSThread* OSThread::_main_thread = nullptr;
OSThread::OSThread() :
        _plib_id(0),
        _kernel_id(0),
        _priority(0),
        _os_state(STATE_NEW),
        _resource_arena(nullptr) {
}


OSThread::~OSThread() {
    delete this->_resource_arena;
}


void OSThread::tans_state(uint8_t to) {
    assert(to != OSThread::STATE_NEW,"This status cannot be set");

    auto from = this->state();
    //ZOMBIE前置状态必须是BLOCKED
    assert((to == STATE_ZOMBIE && from == STATE_BLOCKED) || to != STATE_ZOMBIE,"The ZOMBIE prefix status must be BLOCKED");
    /**
     * 首先设置将线程状态设置成过渡态
     * 方便其他部件即时得知线程状态
     */
    this->_os_state.store(from + 1);
    //立刻刷新到内存 禁止重排序 方便其他线程立刻观察到
    std::atomic_thread_fence(std::memory_order::seq_cst);
    //调用对应的回调函数
    this->state_transitioning_callback(from, to);
    //整体的执行逻辑顺序必须得到保证
    std::atomic_thread_fence(std::memory_order::seq_cst);
    //正式的设置最终的目标状态
    this->_os_state.store(to);
}

void OSThread::global_initialize() {
    assert(this->_resource_arena == nullptr, "error");
    this->_resource_arena = new Arena(MEMFLAG::Thread);
}

void OSThread::print_on(CharOStream *out) const {
    out->print("nid=%d ", os::current_thread_id());
    switch (this->state()) {
        case STATE_NEW:
            out->print("new ");
            break;
        case STATE_READY:
            out->print("ready ");
            break;
        case STATE_RUNNING:
            out->print("running ");
            break;
        case STATE_BLOCKED:
            out->print("blocked");
            break;
        case STATE_ZOMBIE:
            out->print("zombie");
            break;
        default:
            out->print("unknown state %d", this->state());
            break;
    }
    int16_t os_prio = 0;
    if (os::get_native_prio(this->_kernel_id, &os_prio) == OSReturn::OK) {
        out->print(" priority=%d,os_priority=%d",
                   this->get_priority(),
                   os_prio);
    }

}

void *OSThread::native_call(void *params) {
    assert(params != nullptr, "check");
    auto osThread = reinterpret_cast<OSThread *>(params);
    assert(osThread->state() == OSThread::STATE_READY, "thread state is error.");
    //进行前期的
    OSThread::_current = osThread;
    osThread->_kernel_id = os::current_thread_id();
    osThread->_plib_id = ::pthread_self();

    osThread->tans_state(OSThread::STATE_RUNNING);
    std::atomic_thread_fence(std::memory_order::seq_cst);
    osThread->pre_run();
    std::atomic_thread_fence(std::memory_order::seq_cst);
    osThread->run();
    std::atomic_thread_fence(std::memory_order::seq_cst);
    osThread->post_run();
    std::atomic_thread_fence(std::memory_order::seq_cst);
    osThread->tans_state(OSThread::STATE_ZOMBIE);
    return nullptr;
}

void OSThread::attach_main_thread(OSThread *main_thread) {
    assert(main_thread != nullptr, "not null");
    assert(main_thread->state() == OSThread::STATE_NEW, "thread state is error.");
    OSThread::_main_thread = main_thread;

    //调用函数进行初始化
    main_thread->global_initialize();
//    OrderAccess::compile_barrier();
//    OrderAccess::store<uint8_t>(&main_thread->_os_state, OSThread::STATE_READY);
    main_thread->_os_state.store(OSThread::STATE_READY);
    assert(main_thread->state() == OSThread::STATE_READY, "thread state is error.");
    //进行前期的
//    OrderAccess::store<OSThread *>(&OSThread::_current, main_thread);
//    OrderAccess::store<pthread_t>(&main_thread->_plib_id, pthread_self());
//    OrderAccess::store(&main_thread->_kernel_id, OS::current_thread_id());
    OSThread::_current = main_thread;
    main_thread->_kernel_id = os::current_thread_id();
    main_thread->_plib_id = ::pthread_self();
    main_thread->tans_state(OSThread::STATE_RUNNING);
}


ResourceArenaMark::ResourceArenaMark() :
        _arena(OSThread::current()->resource_arena()),
        _saved(_arena) {
    assert(this->_arena != nullptr, "must be not null");
}

ResourceArenaMark::~ResourceArenaMark() {
    this->_saved.rollback_to(this->_arena);
}
