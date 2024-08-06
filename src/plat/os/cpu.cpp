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
#include "plat/os/cpu.hpp"
#include "plat/utils/robust.hpp"
#include "plat/thread/OSThread.hpp"
#include "plat/constants.hpp"
#include "global/flag.hpp"
#include "plat/utils/OrderAccess.hpp"

namespace os {
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
    uint32_t avail_cpu_num() {
        static auto num = (uint32_t) ::sysconf(_SC_NPROCESSORS_ONLN);
        return num;
    }

    uint32_t total_cpu_num() {
        static auto num = (uint32_t) ::sysconf(_SC_NPROCESSORS_CONF);
        return num;
    }

    /**
     * 获取线程在系统中的ID 会缓存
     * @return
     */
    int32_t current_thread_id() {
        thread_local auto tid = ::gettid();
        return tid;
    }

    /**
     * 获取当前系统进程的ID 只有第一次会调用系统
     * @return
     */
    int32_t current_process_id() {
        static auto pid = ::getpid();
        return pid;
    }

    uint32_t current_cpu_id() {
        long cpu;
        auto res = ::syscall(SYS_getcpu, &cpu, nullptr, nullptr);
        return res == 0 ? (uint32_t) cpu : 0;
    }

    OSReturn get_native_prio(int32_t thread_id,
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

    OSReturn set_native_prio(int32_t thread_id,
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

    void native_prio_initialize() {
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

    bool create_thread(OSThread *thread, bool detach) {
        assert(thread != nullptr, "thread is null");
        assert(thread->_os_state == OSThread::STATE_NEW,"check");
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
        OrderAccess::store<uint8_t>(&thread->_os_state, OSThread::STATE_READY);
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
            OrderAccess::store<uint8_t>(&thread->_os_state, OSThread::STATE_ZOMBIE);
            return false;
        }
        return true;
    }

    void join_thread(OSThread *thread) {
        ::pthread_join(thread->get_pthread_id(), nullptr);
    }


    /**
     * 当 *uaddr == tag时 挂起线程
     * @param uaddr
     * @param tag 标志 当此值等于uaddr,则进入睡眠
     * @param nsec 超时等待多少ns 0 表示无限期等待
     */
    void suspend(const int *uaddr, int tag, uint64_t nsec) {
        do {
            struct timespec *spec = nullptr;
            struct timespec spec_val{};
            if (nsec > 0) {
                // 需要从ns中分离出来ns和s
                constexpr auto ns_per_sec = TicksPerS / TicksPerNS;
                uint64_t sec = nsec / ns_per_sec;
                clock_gettime(CLOCK_REALTIME, &spec_val);
                spec_val.tv_sec += (long) sec;
                spec_val.tv_nsec += (long) nsec - (long) sec * ns_per_sec;
                spec = &spec_val;
            }

            const auto state = (int) syscall(SYS_futex,
                                             uaddr,
                                             FUTEX_WAIT_PRIVATE,
                                             tag,
                                             spec);
            guarantee((state == 0) ||
                      (state == -1 && errno == EAGAIN) ||
                      (state == -1 && errno == EINTR),
                      "futex FUTEX_WAIT 失败");
            /**
             * 得到返回0 我们还需要再次进行检查 防止虚假的唤醒
             * 有些错误可以重试 我们也进行重试
             */
        } while (tag == *uaddr);
    }

    /**
     * 唤醒等待在uaddr上的num个线程线程
     * @param uaddr int类型变量
     * @param num 线程的数量(如果大于实际等待的线程数量，则唤醒全部线程)
     * @return 返回实际被唤醒的线程数
     */
    int wakeup(int *uaddr, int num) {
        const auto wake_num = (int) syscall(SYS_futex,
                                            uaddr,
                                            FUTEX_WAKE_PRIVATE,
                                            num,
                                            nullptr);
        guarantee(wake_num >= 0, "futex FUTEX_WAKE 失败");
        return wake_num;
    }

}

