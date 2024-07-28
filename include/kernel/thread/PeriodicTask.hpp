//
// Created by aurora on 2024/4/21.
//

#ifndef KERNEL_THREAD_PERIODIC_TASK_HPP
#define KERNEL_THREAD_PERIODIC_TASK_HPP

#include "plat/mem/allocation.hpp"
#include "kernel/constants.hpp"

/**
 * 周期性任务
 */
class PeriodicTask : public CHeapObject<MEMFLAG::Internal> {
    friend class PeriodicThread;
private:
    /**
     * 用于存储全部定时任务的
     */
    static uint16_t _num_of_tasks;
    static PeriodicTask *_tasks[KernelConstants::PeriodicTaskMaxNum];



    /**
     * 定时任务的执行的周期
     */
   const  uint32_t _interval_count;
    /**
     * 当前计数 注意到到达周期性定时任务执行完毕后会被自动清零
     * 即取值在0~_interval之间
     */
    uint32_t _current_count;

    /**
     * 检查当前时间间隔是否足够
     * 足够定时任务执行
     * @param delay_time 距离上一次检测的时间间隔
     */



protected:
    virtual void task() = 0;

public:
    explicit PeriodicTask(uint32_t interval);

    virtual ~PeriodicTask() {
        this->inactivate();
    };

    /**
     * 激活定时任务
     */
    void activate();

    /**
     * 让当前定时任务停止运行
     */
    void inactivate();

    /**
     * 开启清除arena的定时任务
     */
    static void start_arena_clean_task();
};


#endif //KERNEL_THREAD_PERIODIC_TASK_HPP
