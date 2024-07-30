//
// Created by aurora on 2024/7/28.
//
#include "kernel/KernelInitialize.hpp"
#include "PeriodicThread.hpp"
#include "kernel_mutex.hpp"
#include "VMThread.hpp"
void KernelInitialize::daemon_thread_initialize() {
    //1. 创建周期性任务的守护线程
    PeriodicThread::create();
    VMThread::create();

    PeriodicThread::start();
}

void KernelInitialize::initialize() {
    kernel_mutex_init();
    daemon_thread_initialize();
}
