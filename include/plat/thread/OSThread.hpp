//
// Created by aurora on 2022/12/5.
//

#ifndef PLATFORM_OS_THREAD_HPP
#define PLATFORM_OS_THREAD_HPP

#include "plat/constants.hpp"
#include "plat/utils//OrderAccess.hpp"
#include "plat/utils/robust.hpp"
#include "plat/mem/Arena.hpp"
#include "plat/os/cpu.hpp"

/**
 * NAME 表示线程的一个状态
 * NAME##_TRANS 表示线程状态正在从NAME状态迁移到其他状态 一般在进入安全点检查的时候使用
 */
#define THREAD_STATE_DECL(NAME) \
    STATE_##NAME,STATE_##NAME##_TANS


class Arena;

/**
 * 对应于系统的线程
 * 并且是对其进行抽象
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
    typedef unsigned long thread_id_t;

    friend bool os::create_thread(OSThread *thread, bool detach);
    friend class PlatInitialize;
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
    volatile uint8_t _os_state;
    /**
     * 优先级
     * 1 最低
     * 10 最高
     */
    int8_t _priority;
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
     * 初始化函数
     * 子类如果覆盖的话，需要先调用父类函数
     */
    virtual void global_initialize();
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


    [[nodiscard]] inline auto state() const {
        return OrderAccess::load<uint8_t>(&_os_state);
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
        return OSThread::is_running_state(OrderAccess::load(&this->_os_state));
    };

    [[nodiscard]] inline bool is_tans_state() const {
        return OSThread::is_tans_state(OrderAccess::load(&this->_os_state));
    };

    /**
     * 更新线程的状态
     * @param to 目标的状态，但是不能是中间态
     */
    void tans_state(uint8_t to);

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
    /**
     * 直接设置是阻止的
     * 用于在state_transitioning_callback
     */
    inline void set_blocked_direct(){
        OrderAccess::store<uint8_t>(&this->_os_state,OSThread::STATE_BLOCKED);
    }
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
