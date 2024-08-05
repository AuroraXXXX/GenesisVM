//
// Created by aurora on 2024/4/21.
//

#include "PeriodicThread.hpp"
#include "kernel_mutex.hpp"
#include "kernel/thread/PeriodicTask.hpp"
#include "plat/os/cpu.hpp"
#include "plat/os/time.hpp"

PeriodicThread * volatile PeriodicThread::_periodic_thread = nullptr;
volatile bool PeriodicThread::_should_terminate = false;
bool PeriodicThread::_run_tasks = false;


void PeriodicThread::create() {
    MutexLocker locker(PeriodicTask_lock);
    PeriodicThread::_should_terminate = false;
    assert(PeriodicThread::periodic_thread() == nullptr, "must be");
    const auto thread = new PeriodicThread();
    if (os::create_thread(thread)) {
        PeriodicThread::_periodic_thread = thread;
    } else {
        delete thread;
    }

}
void PeriodicThread::start() {
    MonitorLocker locker(PeriodicTask_lock);

    PeriodicThread::_run_tasks = true;
    locker.notify();
}
void PeriodicThread::run() {
    while (true) {
        assert(this == PeriodicThread::periodic_thread(), "check %x",PeriodicThread::periodic_thread());
        assert(this == PeriodicThread::current(), "check");
        auto delay_interval = PeriodicThread::calculate_next_task_interval();

        if (OrderAccess::load(&PeriodicThread::_should_terminate)) {
            //先查看是否需要中断
            break;
        }
        if (PeriodicThread::_run_tasks) {
            //执行定时任务
            PeriodicThread::check_and_execute(delay_interval);
        }
    }
    {
        //在此时就需要进行线程销毁了
        auto periodic_thread = PeriodicThread::periodic_thread();
        PeriodicThread::_periodic_thread = nullptr;
        delete periodic_thread;
    }
}

void PeriodicThread::stop() {
    MonitorLocker lock(PeriodicTask_lock);
    /**
     * 表示要中止线程
     */
    OrderAccess::store(&PeriodicThread::_should_terminate, true);
    auto periodic_thread = PeriodicThread::periodic_thread();
    if (periodic_thread != nullptr) {
        //唤醒线程 去 中止线程
        lock.notify();
    }

}



uint32_t PeriodicThread::calculate_next_task_interval() {
    if (OrderAccess::load(&PeriodicThread::_should_terminate)) {
        return 0;
    }
    MonitorLocker lock(PeriodicTask_lock);
    if (!PeriodicThread::_run_tasks) {
        //此时还没有执行定时任务 间隔0.1s 检查一次
        lock.wait(PeriodicThread::interval_to_millis(KernelConstants::PeriodicNoRunTaskCheckInterval));
        return 0;
    }
    auto remains = PeriodicThread::min_task_interval();
    uint32_t time_next_interval;
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
            time_next_interval = 0;
        } else {
            //计算出距离下一次任务的间隔
            time_next_interval = (uint32_t) (now - time_before_loop) /
                                 PeriodicThread::unit_ticks();
        }
        if (timeout || PeriodicThread::_should_terminate) {
            //更改到任务列表或某些类型的虚假唤醒
            break;
        }
        remains = PeriodicThread::min_task_interval();
        if (remains == 0) {
            //说明现在没有定时任务，等待其他线程设置周期任务并且激活
            continue;
        }
        if (remains <= time_next_interval) {
            //<=0 说明是到唤醒周期任务的时候了
            break;
        }
        remains -= time_next_interval;
    }
    return time_next_interval;
}

uint32_t PeriodicThread::min_task_interval() {
    assert_lock_strong(PeriodicTask_lock);
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


void PeriodicThread::check_and_execute(uint32_t delay_interval) {
    assert(PlatThread::current()->is_watcher_thread(), "must be");
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
    MutexLocker locker(PeriodicTask_lock);
    auto origin_num_tasks = PeriodicTask::_num_of_tasks;
    for (uint16_t i = 0; i < origin_num_tasks; ++i) {
        //执行定时任务
        const auto task = PeriodicTask::_tasks[i];
        //检查 如果满足条件就进行运行
        execute_if_pending(task,delay_interval);
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