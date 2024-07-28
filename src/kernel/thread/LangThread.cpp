//
// Created by aurora on 2023/12/21.
//

#include "kernel/thread/LangThread.hpp"
#include "safepoint.hpp"
void LangThread::state_transitioning_callback(uint8_t from_state, uint8_t to_state) {
    if (!SafepointSynchronize::is_at_safepoint()) {
        return;
    }
    if (!OSThread::is_running_state(from_state)) {
        return;
    }
    /**
     * 当线程时运行态或者时中间态时
     * 我们需要进行阻塞
     * 1 首先设置成阻塞态
     * 2 当安全点结束后再恢复成原来的状态
     */
    this->set_blocked_direct();
    OrderAccess::compile_barrier();
    SafepointSynchronize::_wait_barrier.wait(SafepointSynchronize::_safe_point_check);
    OrderAccess::compile_barrier();
    assert(SafepointSynchronize::_state != SafepointSynchronize::synchronized,
           "再次得到执行应该是安全点结束");
}

LangThread::LangThread() :
        PlatThread(),
        _still_running_next(nullptr){

}











