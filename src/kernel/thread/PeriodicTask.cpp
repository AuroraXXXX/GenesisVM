//
// Created by aurora on 2024/4/21.
//

#include "kernel/thread/PeriodicTask.hpp"
#include "kernel_mutex.hpp"
#include "kernel/thread/PlatThread.hpp"
#include "kernel/constants.hpp"
uint16_t PeriodicTask::_num_of_tasks = 0;
PeriodicTask *PeriodicTask::_tasks[ KernelConstants::PeriodicTaskMaxNum];

PeriodicTask::PeriodicTask(uint32_t interval) :
        _interval_count((uint32_t) interval),
        _current_count(0) {
    assert(is_clamp(interval,
                    KernelConstants::PeriodicTaskMinInterval ,
                    KernelConstants::PeriodicTaskMaxInterval),
           "PeriodicTask interval must be within min_interval,max_interval");
}




void PeriodicTask::inactivate() {
    MutexLocker locker(PeriodicTask_lock->owned_by_self() ? nullptr : PeriodicTask_lock);
    uint16_t index = 0;
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
    guarantee(PeriodicTask::_num_of_tasks < KernelConstants::PeriodicTaskMaxNum,
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
protected:

    inline void task() override {
        Arena::clean_pool();
    }

public:
    inline explicit ArenaChunkPoolCleanTask() :
            PeriodicTask(KernelConstants::PeriodicTaskArenaClearInterval) {
    }
};
/**
 *  -------------------
 */

void PeriodicTask::start_arena_clean_task() {
    const auto  task = new ArenaChunkPoolCleanTask();
    task->activate();
}
