//
// Created by aurora on 2023/10/21.
//

#ifndef PLATFORM_MUTEX_HPP
#define PLATFORM_MUTEX_HPP

#include <cerrno>
#include <pthread.h>
#include "plat/utils/robust.hpp"
#include "plat/mem/allocation.hpp"
#include "plat/macro.hpp"


class OSThread;

/**
 * 对原本平台的互斥量 的包装，让其符合其他模块调用规范
 *
 */
class Mutex : public CHeapObject<MEMFLAG::Synchronizer> {
protected:
    pthread_mutex_t _mutex;
    const char *const _name;
    NONCOPYABLE(Mutex);

    OSThread *volatile _owner;
public:
    /**
     * @param name 互斥量的名称 但是内部不会拷贝字符串
     * @param recursive 是否是可重入的
     */
    explicit Mutex(const char *name, bool recursive = true) noexcept;

    ~Mutex();

    [[nodiscard]] inline const char *name() const {
        return this->_name;
    };

    /**
     * 锁定
     */
    void lock();

    /**
     * 解除锁定
     */
    void unlock();

    /**
     * 尝试进行锁定
     * @return 锁定是否成功
     */
    bool try_lock();

    [[nodiscard]] inline bool is_locked() const {
        return this->_owner != nullptr;
    };

    /**
     * 判断持有该锁线程的是本线程
     * @return
     */
    bool owned_by_self();

    [[nodiscard]] inline OSThread *owner() const {
        return this->_owner;
    };

    void print_on(CharOStream *stream);
};


#endif //PLATFORM_MUTEX_HPP
