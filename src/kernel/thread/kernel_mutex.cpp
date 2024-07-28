//
// Created by aurora on 2022/12/19.
//

#include "kernel_mutex.hpp"
#include "safepoint.hpp"

#define MUTEX_ENUM(type, name, human_readable)  type* name##_lock = nullptr;

    VM_MUTEX_LIST(MUTEX_ENUM)

#undef MUTEX_ENUM

    /**
     * 初始化虚拟机过程中所使用到的锁
     * 在调用这个之前 线程对象必须完成初始化
     * 即调用
     */
    extern void kernel_mutex_init() {
#define MUTEX_ENUM(type, name, human_readable) name##_lock = new type(#name);
        VM_MUTEX_LIST(MUTEX_ENUM)
#undef  MUTEX_ENUM
    }

#ifdef DEBUG_MODE_ONLY

    extern void assert_lock_strong(Mutex *lock) {
        assert(lock != nullptr, "需要一个不为nullptr的锁");
        assert(lock->owned_by_self(), "必须拥有锁:%s", lock->name());
    }
    extern void assert_locked_or_safepoint(Mutex* lock){
        assert(lock != nullptr, "需要一个不为nullptr的锁");
        if(lock->owned_by_self())return;
        if(SafepointSynchronize::is_at_safepoint()) return;
        assert(false,"必须拥有锁:%s,或者在安全点", lock->name());
    }
#endif
