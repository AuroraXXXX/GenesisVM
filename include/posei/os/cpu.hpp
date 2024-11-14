//
// Created by aurora on 2024/6/24.
//

#ifndef PLATFORM_OS_CPU_HPP
#define PLATFORM_OS_CPU_HPP

#include "stdtype.hpp"
#include "plat/constants.hpp"

class OSThread;
namespace os {

    /**
     * 获取可用的CPU数量
     * @return
     */
    extern uint32_t avail_cpu_num();

    /**
     * 获取全部的CPU数量
     * @return
     */
    extern uint32_t total_cpu_num();

    /**
     * 获取当前线程所在的CPU序号
     * @return
     */
    extern uint32_t current_cpu_id();

    inline bool is_MP() { return avail_cpu_num() != 1; }

    /**
     * 获取线程在系统中的ID 只有第一次会进行调用系统
     * @return
     */
    extern int32_t current_thread_id();

    /**
     * 获取当前系统进程的ID 只有第一次会进行调用系统
     * @return
     */
    extern int32_t current_process_id();

    /**
     * 获取OS本身的线程优先级
     * @param thread_id OS的线程ID
     * @param native_prio OS线程实际设置的线程优先级
     * @return
     */
    extern OSReturn get_native_prio(int32_t thread_id,
                                    int16_t *native_prio);

    /**
     * 设置OS的线程优先级
     * @param thread_id OS的线程ID
     * @param lang_prio ThreadPriority 规定的线程优先级
     * @return 操作状态码
     */
    extern OSReturn set_native_prio(int32_t thread_id,
                                    ThreadPriority lang_prio);

    /**
     * 创建线程
     * @param thread 线程对象
     * @param detach 是否是分离对象 分离表示执行完毕自动销毁
     * @return
     */
    bool create_thread(OSThread *thread, bool detach = true);

    /**
     * 等待线程
     * @param thread
     */
    void join_thread(OSThread *thread);
}
#endif //PLATFORM_OS_CPU_HPP
