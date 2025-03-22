//
// Created by aurora on 2024/4/21.
//

#ifndef PLATFORM_PERIODIC_TASK_HPP
#define PLATFORM_PERIODIC_TASK_HPP

#include "platform/allocation.hpp"
#include "platform/constants.hpp"

class Monitor;

/**
 * 周期性任务
 */
class PeriodicTask : public CHeapObject<MEMFLAG::Internal> {
    friend class PeriodicThread;

public:
    /**
     * 支持的最大的定时任务个数
     */
    constexpr inline static uint16_t MAX_TASKS_NUM = 10;
    /**
     * 最大允许的定时任务，执行时间是10s= MAX_INSPECT_INTERVAL_COUNT * INSPECT_INTERVAL *1ns
     */
    constexpr inline static uint16_t MAX_INSPECT_INTERVAL_COUNT = 1E3;
    /**
     * 定时任务检查的间隔单位(单位是纳秒),表示 10ms 。
     */
    constexpr inline static uint32_t INSPECT_INTERVAL = 10 * TicksPerMS;
    /**
     * 没有任务的时候，检查的间隔 10*10ms = 0.1s
     */
    constexpr inline static uint16_t NO_RUN_INSPECT_INTERVAL_COUNT = 10;

private:


    /**
     * 用于存储全部定时任务的
     */
    static uint16_t _num_of_tasks;
    static PeriodicTask *_tasks[MAX_TASKS_NUM];
    /**
     * 用于定时任务内部管理的互斥锁
     */
    static Monitor *PeriodicTask_lock;

    /**
     * 定时任务的执行的间隔，单位是10ms
     *
     */
    const uint32_t _interval_count;
    /**
     * 当前计数 注意到到达周期性定时任务执行完毕后会被自动清零
     * 即取值在0~_interval之间
     */
    uint32_t _current_count;


protected:
    /**
     * 具体的定时任务
     */
    virtual void task() = 0;

public:
    /**
     * 获取锁定的对象
     * @return
     */
    static inline Monitor *locker() {
        return PeriodicTask::PeriodicTask_lock;
    };

    /**
     * 定时任务的构造函数
     * @param interval_count 检查的间隔数
     */
    explicit PeriodicTask(uint32_t interval_count);

    /**
     * 定时任务的析构函数，取消定时任务的执行
     */
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


#endif //PLATFORM_PERIODIC_TASK_HPP
