//
// Created by aurora on 2022/12/5.
//

#ifndef PLATFORM_OS_THREAD_HPP
#define PLATFORM_OS_THREAD_HPP

#include "platform/constants.hpp"
#include <atomic>
#include "platform/utils/robust.hpp"
#include "platform/mem/Arena.hpp"
#include "platform/os.hpp"
#include "platform/utils/LinkStack.hpp"
/**
 * NAME 表示线程的一个状态
 * NAME##_TRANS 表示线程状态正在从NAME状态迁移到其他状态 一般在进入安全点检查的时候使用
 */
#define THREAD_STATE_DECL(NAME) \
    STATE_##NAME,STATE_##NAME##_TANS


class Arena;
class Monitor;
class Mutex;

/**
 * 对应于系统的线程
 * 并且是对其进行抽象
 *
 * 只有2个子类：
 * - UserThread 用户线程
 * - NonUserThread 内核线程(虚拟机内部线程)，注意一定是用户线程
 *
 */
class OSThread : public CHeapObject<MEMFLAG::Thread> {
public:
    /**
     * 线程的状态
     * 新增的线程状态必须使用 THREAD_STATE_DECL 宏进行声明
     * 且声明在 NEW 和 ZOMBIE 之后
     */
    enum : uint8_t {
        STATE_NEW = 0,               // 线程对象刚被创建出来
        STATE_ZOMBIE,                // 一切都完成了，但还没有被收回 即此时还没有从对应的链表中移除
        THREAD_STATE_DECL(READY),     // 线程对象各种资源准备完毕，等待CPU调度
        THREAD_STATE_DECL(RUNNING),   // 线程对象已经被CPU调度
        THREAD_STATE_DECL(BLOCKED)    // 线程对象已经被阻塞 停止运行了
    };
private:

    friend void platform_init();

    friend void platform_destroy();

    typedef unsigned long thread_id_t;

    friend class os;

    /**
     * pthread库调用OSTread中的函数，进行触发OSThread中的run函数
     * @param thread
     */
    static void *native_call(void *params);

    static OSThread *_main_thread;

    /**
     * 将main对象进行绑定
     * @param main_thread
     */
    static void attach_main_thread(OSThread *main_thread);

public:
    static inline auto main_thread() {
        return OSThread::_main_thread;
    };
private:
    /**
     * 线程的状态
     */
    std::atomic<uint8_t> _os_state;
    /**
     * 优先级
     * 1 最低
     * 10 最高
     */
    int8_t _priority;
    /**
     * OS内核线程的ID
     */
    int32_t _kernel_id;
    /**
     * 线程库中的线程ID
     */
    thread_id_t _plib_id;

    /**
     * 内部的资源区域
     */
    Arena *_resource_arena;
    thread_local static OSThread *_current;
    NONCOPYABLE(OSThread);

protected:
    /**
     * 进行状态过渡的回调函数
     * @param from_state 原本的状态
     * @param to_state 目标状态
     */
    virtual void state_transitioning_callback(
            uint8_t from_state,
            uint8_t to_state) {};


    /**
     * 直接设置是阻止的
     * 用于在state_transitioning_callback
     */
    inline void set_blocked_direct() {
        this->_os_state.store(OSThread::STATE_BLOCKED);
    }

public:
    static inline OSThread *current() {
        return OSThread::_current;
    };

    /**
     *
     */
    explicit OSThread();

    virtual ~OSThread();


    inline Arena *resource_arena() {
        return this->_resource_arena;
    };

    /**
     * 获取线程的ID，这里是pthread库的id
     * @return
     */
    [[nodiscard]] inline thread_id_t get_pthread_id() const {
        return this->_plib_id;
    };

    [[nodiscard]] inline auto get_kernel_id() const {
        return this->_kernel_id;
    };

    [[nodiscard]] inline auto get_priority() const {
        return this->_priority;
    };

    /**
     * 获取线程的状态
     * @return
     */
    [[nodiscard]] inline auto state() const {
        return this->_os_state.load();
    };

    /**
     * 是否是线程的过度状态
     * 注意这个需要根据线程状态的声明顺序来决定
     * @param state
     * @return
     */
    static bool is_tans_state(uint8_t state) {
        return state > 1 && (state & 1);
    };

    /**
     * 判断线程状态是否是运行态
     * @param state
     * @return
     */
    static inline bool is_running_state(uint8_t state) {
        return state == STATE_RUNNING;
    };

    [[nodiscard]] inline bool is_running_state() const {
        return OSThread::is_running_state(this->state());
    };

    [[nodiscard]] inline bool is_tans_state() const {
        return OSThread::is_tans_state(this->state());
    };

    /**
     * 所属的类型名
     * @return
     */
    virtual const char *name() = 0;

    /**
     * 该对象是不是 用户线程
     * @return
     */
    virtual bool is_user_thread() = 0;

    /**
     * 该对象是不是 守护线程
     * @return
     */
    virtual bool is_daemon_thread() = 0;

    /**
     * 更新线程的状态
     * @param to 目标的状态，但是不能是中间态
     */
    void tans_state(uint8_t to);
    /**
     * 打印线程信息
     * @param out
     */
    virtual void print_on(CharOStream *out) const;

protected:
    /**
     * 先执行的方法
     */
    virtual void pre_run() = 0;

    /**
     * 正式执行的方法
     */
    virtual void run() = 0;

    /**
     * 后执行的方法
     */
    virtual void post_run() = 0;

};


/**
 * 表示用户线程 ，支持放入到用户线程链表中
 */
class UserThread : public OSThread {
    friend class LinkStack<UserThread>;

private:
    /**
     * 保证线程安全的锁
     */
    static Monitor *_locker;
    static LinkStack<UserThread> _list;
    /**
     * 用户线程中守护线程的数量
     */
    static size_t _non_daemon_of_user_thread_count;

    std::atomic<UserThread *> _next;
    /**
     * 用于检测用户线程是否存活（在检测进入安全点的时候使用）
     */
    UserThread *_stilling_next;

    inline void set_next(UserThread *next) {
        this->_next = next;
    };

    inline UserThread *next() {
        return this->_next;
    }

protected:
    void pre_run() override;

    /**
     * 执行完毕后执行该函数
     */
    void post_run() override;

public:
    /**
     * 表示用户线程
     * @return
     */
    bool is_user_thread()  final{
        return true;
    };

    /***
     * 表示不是守护线程
     * @return
     */
    bool is_daemon_thread() override {
        return false;
    };

    /**
     * 锁定链表，不允许新创建的线程对象添加到链表，那么就不会执行用户的run函数中的代码
     */
    static inline Monitor *locker() {
        return UserThread::_locker;
    };

    /**
     * 获取链表
     * @return
     */
    static inline auto &list() {
        return UserThread::_list;
    };

    /**
     * 对象类型名
     * @return
     */
    const char *name() override;

    explicit UserThread();

    inline void set_stilling_next(UserThread *next) {
        this->_stilling_next = next;
    };

    inline auto stilling_next() {
        return this->_stilling_next;
    };

   static inline auto non_daemon_of_user_thread_count(){
       return UserThread::_non_daemon_of_user_thread_count;
    };
};

/**
 * 表示main线程
 */
class MainThread : public UserThread {
protected:
    void run() override;
};

/**
 * 非用户线程 支持放入到 链表中
 * 非用户线程一定是 守护线程
 */
class NonUserThread : public OSThread {
    friend class LinkStack<NonUserThread>;

private:
    /**
     * 保证线程安全的锁
     */
    static Mutex *_locker;
    static LinkStack<NonUserThread> _list;
    std::atomic<NonUserThread *> _next;

    inline void set_next(NonUserThread *next) {
        this->_next.store(next);
    };

    inline auto next() {
        return this->_next.load();
    }

protected:

    void pre_run() override;


    void post_run() override;

public:
    /**
     *  表示非用户线程，并且所有子类都不应该重新的覆盖
     * @return
     */
    bool is_user_thread() final {
        return false;
    };
    /**
     * 表示是守护线程，但是
     * @return
     */
    bool is_daemon_thread() final {
        return true;
    };

    const char *name() override;

    explicit NonUserThread();

};

/**
 * OSThread 的 resource mem
 */
class ResourceArenaMark : public StackObject {
private:
    Arena *_arena;
    Arena::SavedData _saved;
public:
    explicit ResourceArenaMark();

    ~ResourceArenaMark();
};

#endif //PLATFORM_OS_THREAD_HPP
