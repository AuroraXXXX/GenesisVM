//
// Created by aurora on 2024/4/21.
//

#include "PeriodicThread.hpp"
#include "platform/concurrent/PeriodicTask.hpp"
#include "platform/concurrent/Monitor.hpp"
#include "platform/log.hpp"

PeriodicThread *PeriodicThread::_periodic_thread = nullptr;

PeriodicThread::PeriodicThread() :
        _periodic_state(PeriodicState::creating) {

}


void PeriodicThread::create() {
    MutexLocker locker(PeriodicTask::locker());
    assert(PeriodicThread::_periodic_thread == nullptr, "must be");
    PeriodicThread::_periodic_thread = new PeriodicThread();
    if (os::create_thread(PeriodicThread::_periodic_thread)) {
        log_info(nonuserthread)("PeriodicThread created is success");
    } else {
        delete PeriodicThread::_periodic_thread;
        PeriodicThread::_periodic_thread = nullptr;
    }

}

void PeriodicThread::start() {
    MonitorLocker locker(PeriodicTask::locker());
    //调整状态到执行
    PeriodicThread::_periodic_thread->_periodic_state.store(PeriodicState::running);
    locker.notify();
}

void PeriodicThread::stop() {
    {
        MonitorLocker lock(PeriodicTask::locker());
        log_info(nonuserthread)("PeriodicThread stopping...");
        /**
         * 表示要中止线程
         */
        auto thread = PeriodicThread::_periodic_thread;
        thread->_periodic_state.store(PeriodicState::should_terminate);
        //唤醒线程 去 中止线程
        lock.notify();
        std::atomic_thread_fence(std::memory_order::seq_cst);
        //当前线程需要等待 PeriodicThread 进行唤醒
        lock.wait();
        log_info(nonuserthread)("PeriodicThread stopped...");
    }

}

void PeriodicThread::run() {
    while (true) {
        assert(PeriodicThread::is_periodic_thread_caller(), "check");
        auto delay_interval = PeriodicThread::calculate_next_task_interval();

        if (this->is_target_state(PeriodicState::should_terminate)) {
            //先查看是否需要中断
            break;
        }
        if (this->is_target_state(PeriodicState::running)) {
            //执行定时任务
            PeriodicThread::execute(delay_interval);
        }
    }
    {
        //进行退出操作
        MonitorLocker ml(PeriodicTask::locker());
        const auto thread = PeriodicThread::_periodic_thread;
        thread->_periodic_state.store(PeriodicState::terminated);
        ml.notify_all();
    }

}


uint32_t PeriodicThread::calculate_next_task_interval() {
    if (this->is_target_state(PeriodicState::should_terminate)) {
        //已经进行了 终止操作，那么就不需要执行后续的操作
        return 0;
    }
    MonitorLocker lock(PeriodicTask::locker());
    if (!this->is_target_state(PeriodicState::running)) {
        //此时还没有执行定时任务 间隔0.1s 检查一次
        constexpr auto wait_inspect_count = (PeriodicTask::INSPECT_INTERVAL / TicksPerMS) * PeriodicTask::NO_RUN_INSPECT_INTERVAL_COUNT;
        lock.wait(wait_inspect_count);
        return 0;
    }
    //获取任务的最小的间隔
    auto remains = PeriodicThread::min_task_interval();
    uint32_t time_next_interval_count;
    //GC前获取时间戳
    auto time_before_loop = os::current_stamp();

    while (true) {
        //等待一会
        bool timeout = lock.wait(remains);
        //再次获取当前的时间
        const auto now = os::current_stamp();
        if (remains == 0) {
            /**
             * 如果我们没有任何任务，可能会等待很长时间，
             * 考虑 time_next_interval 为零并重置time_before_loop
             */
            time_before_loop = now;
            time_next_interval_count = 0;
        } else {
            //计算出距离下一次任务的间隔
            time_next_interval_count = (uint32_t) (now - time_before_loop) /
                                 PeriodicTask::INSPECT_INTERVAL;
        }
        if (timeout || this->is_target_state(PeriodicState::should_terminate)) {
            //判断是福不是呀
            break;
        }
        remains = PeriodicThread::min_task_interval();
        if (remains == 0) {
            //说明现在没有定时任务，等待其他线程设置周期任务并且激活
            continue;
        }
        if (remains <= time_next_interval_count) {
            //<=0 说明是到唤醒周期任务的时候了
            break;
        }
        remains -= time_next_interval_count;
    }
    return time_next_interval_count;
}

uint32_t PeriodicThread::min_task_interval() {
    assert_lock_strong(PeriodicTask::locker());
    if (PeriodicTask::_num_of_tasks == 0) {
        return 0;
    }
    uint32_t delay = -1;
    for (uint16_t index = 0; index < PeriodicTask::_num_of_tasks; ++index) {
        const auto task = PeriodicTask::_tasks[index];
        auto next_interval = task->_interval_count - task->_current_count;
        delay = MIN2(next_interval, delay);
    }
    return delay;

}


void PeriodicThread::execute(uint32_t delay_interval) {
    assert(PeriodicThread::is_periodic_thread_caller(), "must be");
    /**
     * 用于检查
     * 是否满足任务的执行条件
     */
    const auto execute_if_pending =
            [](PeriodicTask *task,
               uint32_t delay_interval) {
                //计算全部使用uint64_t 防止溢出
                const auto tmp =
                        task->_current_count + (ticks_t) delay_interval;
                if (tmp >= task->_interval_count) {
                    //表示 超过定时任务的间隔 需要执行定时任务
                    task->_current_count = 0;
                    task->task();
                } else {
                    //没有超过 那么就增加计数
                    task->_current_count = tmp;
                }
            };
    MutexLocker locker(PeriodicTask::locker());
    auto origin_num_tasks = PeriodicTask::_num_of_tasks;
    for (uint16_t i = 0; i < origin_num_tasks; ++i) {
        //执行定时任务
        const auto task = PeriodicTask::_tasks[i];
        //检查 如果满足条件就进行运行
        execute_if_pending(task, delay_interval);
        if (PeriodicTask::_num_of_tasks < origin_num_tasks) {
            /**
             * 当task中止自身 不需要执行了，会将task从数组中删除，修改计数
             *
             * 情况1：其他线程调用由于存在PeriodicTask_lock，当执行到此方法时，
             * 会获取最新的需要检查和执行的任务链表
             *
             * 情况2：在task任务内部调用了，所在就需要检查执行完毕后，还需要再次查看计数
             * 是否修改，修改了，说明当前索引是全新的task，索引需要回退
             */
            i--;
            origin_num_tasks = PeriodicTask::_num_of_tasks;
        }
    }
}