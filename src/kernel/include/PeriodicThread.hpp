//
// Created by aurora on 2024/4/21.
//

#ifndef KERNEL_THREAD_PERIODIC_THREAD_HPP
#define KERNEL_THREAD_PERIODIC_THREAD_HPP

#include "kernel/thread/PlatThread.hpp"
#include "kernel/constants.hpp"

/**
 * 负责执行周期性的任务
 */
class PeriodicThread : public PlatThread {
private:
    static PeriodicThread * volatile _periodic_thread;
    /**
     * 是否应该中止线程的运行
     */
    volatile static bool _should_terminate;
    /**
     * 是否开始执行定时任务
     */
    static bool _run_tasks;
protected:
    /**
     * 执行定期的任务
     */
    void run() override;

    /**
     * 计算需要多长时间才能完成下一个PeriodicTask工作，并占用这段时间。
     * @return
     */
    [[nodiscard]] static uint32_t calculate_next_task_interval();

    /**
     * 通过所有任务获取线程最小睡眠间隔
     * @return 0表示没有任何周期任务 需要进行永久睡眠
     */
    static uint32_t min_task_interval();
    /**
     * 必须由WatcherThread调用
     * 检查和调用周期任务
     * @param delay_interval 距离上一次检查的时间间隔
     */
    static void check_and_execute(uint32_t delay_interval);
    static constexpr inline ticks_t interval_to_millis(uint32_t interval) {
        return KernelConstants::PeriodicTaskInternalUnit * interval;
    };
    /**
     * 间隔单元对应的ticks数
     * @return
     */
    static constexpr inline ticks_t unit_ticks(){
        return KernelConstants::PeriodicTaskInternalUnit * TicksPerMS;
    };
public:
    bool is_watcher_thread() override {
        return true;
    };

    static inline auto periodic_thread() {
        return PeriodicThread::_periodic_thread;
    };

    const char *name() override {
        return "PeriodicThread";
    };

    static void create();

    /**
     * 停止实际的任务执行
     */
    static void stop();

    /**
     * 开始实际的执行周期任务的执行
     */
    static void start();

    bool is_user_thread() override {
        return false;
    };

    bool is_daemon_thread() override {
        return true;
    };

};


#endif //KERNEL_THREAD_PERIODIC_THREAD_HPP
