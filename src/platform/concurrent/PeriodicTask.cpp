//
// Created by aurora on 2024/4/21.
//

#include "platform/concurrent/PeriodicTask.hpp"
#include "platform/macro.hpp"
#include "platform/utils/robust.hpp"
#include "platform/concurrent/Monitor.hpp"
#include "platform/mem/Arena.hpp"
uint16_t PeriodicTask::_num_of_tasks = 0;
PeriodicTask *PeriodicTask::_tasks[PeriodicTask::MAX_TASKS_NUM];
Monitor* PeriodicTask::PeriodicTask_lock  = new Monitor("PeriodicTask_lock");
PeriodicTask::PeriodicTask(uint32_t interval_count) :
        _interval_count(interval_count),
        _current_count(0) {
    assert(is_clamp<uint32_t>(interval_count,1,PeriodicTask::MAX_INSPECT_INTERVAL_COUNT),
           "PeriodicTask interval must be within min_interval,max_interval");
}


void PeriodicTask::inactivate() {
    //要求在锁的情况下 进行操作，保证数据的一致性
    MutexLocker locker(PeriodicTask_lock->owned_by_self() ? nullptr : PeriodicTask_lock);
    uint16_t index = 0;
    //1 寻找数组中是否存在 对应的 task
    while (index < PeriodicTask::_num_of_tasks) {
        if (PeriodicTask::_tasks[index] == this) {
            break;
        }
        index++;
    }
    if (index == PeriodicTask::_num_of_tasks) {
        //没找到 返回
        return;
    }
    //将index从数组上删除
    PeriodicTask::_num_of_tasks--;
    //将后续的节点移动到前面
    for (; index < PeriodicTask::_num_of_tasks; index++) {
        PeriodicTask::_tasks[index] = PeriodicTask::_tasks[index + 1];
    }
}

void PeriodicTask::activate() {
    MutexLocker locker(PeriodicTask_lock->owned_by_self() ? nullptr : PeriodicTask_lock);
    guarantee(PeriodicTask::_num_of_tasks < PeriodicTask::MAX_TASKS_NUM,
              "Overflow in PeriodicTask table");
    PeriodicTask::_tasks[PeriodicTask::_num_of_tasks++] = this;
    //唤醒睡眠在
    PeriodicTask_lock->notify();
}

/**
 * ------------------
 *  清理Arena的定时任务 ArenaClean
 * ------------------
 */
class ArenaChunkPoolCleanTask : public PeriodicTask {
    /**
     * 检查的间隔 5S
     */
    constexpr inline static uint16_t CLEAR_INTERVAL = 500;
protected:

    inline void task() override {
        Arena::clean_pool();
    }

public:
    inline explicit ArenaChunkPoolCleanTask() :
            PeriodicTask(ArenaChunkPoolCleanTask::CLEAR_INTERVAL) {
    }
};

/**
 *  -------------------
 */

void PeriodicTask::start_arena_clean_task() {
    const auto task = new ArenaChunkPoolCleanTask();
    task->activate();
}
