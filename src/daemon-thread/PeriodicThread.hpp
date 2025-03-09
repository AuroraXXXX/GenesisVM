//
// Created by aurora on 2024/4/21.
//

#ifndef DAEMON_THREAD_PERIODIC_THREAD_HPP
#define DAEMON_THREAD_PERIODIC_THREAD_HPP

#include "platform/concurrent/OSThread.hpp"

/**
 * 负责执行周期性的任务
 */
class PeriodicThread : public DaemonThread {
private:
    static PeriodicThread *  _periodic_thread;
    enum class PeriodicState{
        creating,
        running,
        should_terminate,
        terminated
    };
    std::atomic<PeriodicState> _periodic_state;
    inline bool is_target_state(PeriodicState state){
        return this->_periodic_state.load() == state;
    }
protected:
    /**
     * 执行定期的任务
     */
    void run() override;

    /**
     * 计算需要多长时间才能完成下一个PeriodicTask工作，并占用这段时间。
     * @return 0 表示
     */
     uint32_t calculate_next_task_interval();

    /**
     * 通过所有任务获取线程最小睡眠间隔
     * @return 0表示没有任何周期任务 需要进行永久睡眠
     */
    static uint32_t min_task_interval();
    /**
     * 必须由PeriodicThread调用
     * 检查和调用周期任务
     * @param delay_interval 距离上一次检查的时间间隔
     */
    static void execute (uint32_t delay_interval);

    explicit PeriodicThread();
public:
    /**
     * 判断调用者所在的线程对象是
     * @return
     */
    static bool is_periodic_thread_caller() {
        return PeriodicThread::_periodic_thread == OSThread::current();
    };


    const char *name() override {
        return "PeriodicThread";
    };

    static void create();

    /**
     * 停止实际的任务执行，必须在start之后调用，且调用后不允许调用start
     */
    static void stop();

    /**
     * 开始实际的执行周期任务的执行
     */
    static void start();


};


#endif //DAEMON_THREAD_PERIODIC_THREAD_HPP
