//
// Created by aurora on 2022/10/17.
//
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <syscall.h>
#include <linux/futex.h>
#include <cerrno>
#include <sys/resource.h>
#include "platform/os.hpp"
#include "platform/utils/robust.hpp"
#include "platform/thread/OSThread.hpp"
#include "platform/constants.hpp"
#include "platform/main/global.hpp"

static int16_t LANG_TO_OS_PRIO[TotalPriority] = {
        19,             //从不使用

        4,              // 1 MinPriority
        3,              // 2
        2,              // 3

        1,              // 4
        0,              // 5 NormPriority
        -1,              // 6

        -2,              // 7
        -3,              // 8
        -4,              // 9 NearMaxPriority

        -5,              // 10 MaxPriority
};

/**
 * 返回 可用的cpu的数量
 * @return
 */
uint32_t os::avail_cpu_num() {
    static auto num = (uint32_t) ::sysconf(_SC_NPROCESSORS_ONLN);
    return num;
}

uint32_t os::total_cpu_num() {
    static auto num = (uint32_t) ::sysconf(_SC_NPROCESSORS_CONF);
    return num;
}

/**
 * 获取线程在系统中的ID 会缓存
 * @return
 */
int32_t os::current_thread_id() {
    thread_local auto tid = ::gettid();
    return tid;
}

/**
 * 获取当前系统进程的ID 只有第一次会调用系统
 * @return
 */
int32_t os::current_process_id() {
    static auto pid = ::getpid();
    return pid;
}

uint32_t os::current_cpu_id() {
    long cpu;
    auto res = ::syscall(SYS_getcpu, &cpu, nullptr, nullptr);
    return res == 0 ? (uint32_t) cpu : 0;
}

OSReturn os::get_native_prio(int32_t thread_id,
                             int16_t *native_prio) {
    if (!global::UseThreadPriority) {
        //如果没有使用
        *native_prio = LANG_TO_OS_PRIO[NormPriority];
        return OSReturn::OK;
    }
    errno = 0;
    const auto os_prio = ::getpriority(PRIO_PROCESS, thread_id);
    if (os_prio == -1 && errno) {
        //表示出错
        return OSReturn::ERR;
    }
    int16_t p;
    if (LANG_TO_OS_PRIO[MaxPriority] > LANG_TO_OS_PRIO[MinPriority]) {
        for (p = MaxPriority; p > MinPriority && LANG_TO_OS_PRIO[p] > os_prio; p--);
    } else {
        // niceness values are in reverse order
        for (p = MaxPriority; p > MinPriority && LANG_TO_OS_PRIO[p] < os_prio; p--);
    }
    *native_prio = LANG_TO_OS_PRIO[p];
    return OSReturn::OK;
}

OSReturn os::set_native_prio(int32_t thread_id,
                             ThreadPriority lang_prio) {
    if (!is_clamp<int16_t>(lang_prio, ThreadPriority::MinPriority, ThreadPriority::MaxPriority)) {
        assert(false, "Should not happen");
        return OSReturn::ERR;
    }
    const auto os_prio = LANG_TO_OS_PRIO[lang_prio];
    if (!global::UseThreadPriority) {
        return OSReturn::OK;
    }
    auto res = ::setpriority(PRIO_PROCESS, thread_id, os_prio);
    return res == 0 ? OSReturn::OK : OSReturn::ERR;
}

void os::native_prio_initialize() {
    if (!global::UseThreadPriority) {
        return;
    }
#define NATIVE_PRIO_DEFINE(lv) \
        if(global::ThreadPriority##lv != -1){  \
            LANG_TO_OS_PRIO[lv] = global::ThreadPriority##lv;\
        }
    NATIVE_PRIO_DEFINE(1)
    NATIVE_PRIO_DEFINE(2)
    NATIVE_PRIO_DEFINE(3)
    NATIVE_PRIO_DEFINE(4)
    NATIVE_PRIO_DEFINE(5)
    NATIVE_PRIO_DEFINE(6)
    NATIVE_PRIO_DEFINE(7)
    NATIVE_PRIO_DEFINE(8)
    NATIVE_PRIO_DEFINE(9)
    NATIVE_PRIO_DEFINE(10)
#undef NATIVE_PRIO_DEFINE
}

bool os::create_thread(OSThread *thread, bool detach) {
    assert(thread != nullptr, "thread is null");
    assert(thread->_os_state == OSThread::STATE_NEW, "check");
    //初始化线程属性 以及将系统线程声明为分离线程 这样可以防止内存泄露
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (detach) {
        ::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    } else {
        ::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }
    //进行初始化
    thread->global_initialize();
    pthread_t tid;
    //state 状态调整
    thread->_os_state.store(OSThread::STATE_READY);
    /**
     * 线程创建
     */
    int ret = ::pthread_create(&tid,
                               &attr,
                               OSThread::native_call,
                               thread);
    //销毁 线程属性
    ::pthread_attr_destroy(&attr);
    if (ret != 0) {
        //线程创建失败 那么将线程标记为结束
        thread->_os_state.store(OSThread::STATE_ZOMBIE);
        return false;
    }
    return true;
}

void os::join_thread(OSThread *thread) {
    ::pthread_join(thread->get_pthread_id(), nullptr);
}






