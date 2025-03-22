//
// Created by aurora on 2022/12/5.
//

#include "platform/concurrent/OSThread.hpp"
#include "platform/utils/robust.hpp"
#include "platform/stream/CharOStream.hpp"
#include "platform/os.hpp"
#include <pthread.h>
#include "platform/concurrent/Mutex.hpp"

#include "platform/concurrent/Monitor.hpp"

thread_local OSThread *OSThread::_current = nullptr;
OSThread *OSThread::_main_thread = nullptr;

OSThread::OSThread() :
        _plib_id(0),
        _kernel_id(0),
        _priority(0),
        _os_state(STATE_NEW),
        _resource_arena(nullptr) {
    assert(this->_resource_arena == nullptr, "error");
    this->_resource_arena = new Arena(MEMFLAG::Thread);
}


OSThread::~OSThread() {
    delete this->_resource_arena;
}


void OSThread::tans_state(uint8_t to) {
    assert(to != OSThread::STATE_NEW, "This status cannot be set");

    auto from = this->state();
    //ZOMBIE前置状态必须是BLOCKED
    assert((to == STATE_ZOMBIE && from == STATE_BLOCKED) || to != STATE_ZOMBIE,
           "The ZOMBIE prefix status must be BLOCKED");
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
    assert(osThread->state() == OSThread::STATE_READY, "concurrent state is error.");
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
    assert(main_thread->state() == OSThread::STATE_NEW, "concurrent state is error.");
    OSThread::_main_thread = main_thread;

    //调用函数进行初始化
    main_thread->_os_state.store(OSThread::STATE_READY);
    assert(main_thread->state() == OSThread::STATE_READY, "concurrent state is error.");
    //进行前期的
    OSThread::_current = main_thread;
    main_thread->_kernel_id = os::current_thread_id();
    main_thread->_plib_id = ::pthread_self();
    main_thread->tans_state(OSThread::STATE_RUNNING);
    std::atomic_thread_fence(std::memory_order::seq_cst);
    //将其放入电表中
    main_thread->pre_run();
}


ResourceArenaMark::ResourceArenaMark() :
        _arena(OSThread::current()->resource_arena()),
        _saved(_arena) {
    assert(this->_arena != nullptr, "must be not null");
}

ResourceArenaMark::~ResourceArenaMark() {
    this->_saved.rollback_to(this->_arena);
}

/**
 * ----------------
 * UserThread
 * ----------------
 */
Monitor *UserThread::_locker = new Monitor("user-concurrent-list");
LinkStack<UserThread> UserThread::_list;
size_t UserThread::_non_daemon_of_user_thread_count = 0;

void UserThread::pre_run() {
    MutexLocker locker(UserThread::_locker);
    UserThread::_list.push(this);
    if (!this->is_daemon_thread()) {
        UserThread::_non_daemon_of_user_thread_count++;
    }


}

void UserThread::post_run() {
    MonitorLocker locker(UserThread::_locker);
    //1. 从链表中删除
    UserThread::_list.remove(this);
    //2. 判断是不是守护线程 如果不是要减少相应的计数
    if (!this->is_daemon_thread()) {
        UserThread::_non_daemon_of_user_thread_count--;
    }
    //3. 当UserThread中最后一个非守护线程，需要唤醒可能的等待在 UserThread::_locker上的线程。
    if (UserThread::_non_daemon_of_user_thread_count <= 1) {
        locker.notify_all();
    }

}

UserThread::UserThread() :
        _next(nullptr),
        _stilling_next(nullptr),
        OSThread() {
}

const char *UserThread::name() {
    return "UserThread";
}


/**
 * ----------------
 * DaemonThread
 * ----------------
 */
Mutex *NonUserThread::_locker = new Mutex("daemon-concurrent-list");
LinkStack<NonUserThread> NonUserThread::_list;

void NonUserThread::pre_run() {
    MutexLocker locker(NonUserThread::_locker);
    NonUserThread::_list.push(this);
}

void NonUserThread::post_run() {
    MutexLocker locker(NonUserThread::_locker);
    NonUserThread::_list.remove(this);
}

NonUserThread::NonUserThread() :
        _next(nullptr) {

}

const char *NonUserThread::name() {
    return "NonUserThread";
}


void MainThread::run() {
    //表示的main 线程 ，该函数不应该被调用
    should_not_reach_here();
}
