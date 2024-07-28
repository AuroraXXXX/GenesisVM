//
// Created by aurora on 2022/12/19.
//

#ifndef VMACHINE_VM_MUTEX_LOCKER_HPP
#define VMACHINE_VM_MUTEX_LOCKER_HPP

#include "plat/thread/Monitor.hpp"
#include "plat/thread/Mutex.hpp"
#include "kernel/utils/locker.hpp"


#ifdef DEBUG_MODE_ONLY

    /**
     * 断言该线程已经获取了锁
     * @param mutex
     */
    extern void assert_lock_strong(Mutex *lock);

    extern void assert_locked_or_safepoint(Mutex *lock);

#else
#define assert_lock_strong(lock)
#define assert_locked_or_safepoint(lock)
#endif

#define VM_MUTEX_LIST(f)                                            \
f(Mutex,Metaspace,"元空间全局扩展的锁")                                \
f(Monitor,Heap,"语言层面堆的锁")                                   \
f(Monitor,LangThreadList,"线程控制器 用于控制线程的分配和销毁")      \
f(Mutex,Expand_Heap,"扩展heap时候的锁")                             \
f(Mutex,Safepoint,"安全点设置的锁")                                 \
f(Mutex,NonLangThreadList,"NonLangThreadsList添加和修改的锁")             \
f(Monitor,VMOperation,"系统操作的锁")\
f(Monitor,PeriodicTask,"周期任务的锁")


/**
 * f(Mutex,ClassLoaderDataGraph,"ClassLoaderDataGraph")              \
 * f(Monitor,SystemDictionary,"系统类字典相关的锁")\
f(Mutex,SymbolArena,"根加载的符号存储区域锁")\
 */

#define MUTEX_ENUM(type, name, human_readable) extern type* name##_lock;

    VM_MUTEX_LIST(MUTEX_ENUM)

#undef MUTEX_ENUM

    extern void kernel_mutex_init();

#endif //VMACHINE_VM_MUTEX_LOCKER_HPP
