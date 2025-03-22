//
// Created by aurora on 2024/6/29.
//

#ifndef PLATFORM_MAIN_INIT_HPP
#define PLATFORM_MAIN_INIT_HPP

#include "platform/typedef.hpp"
#include "platform/mem/AllStatic.hpp"

class OSThread;

class Platform {
public:
    /**
     * 进行初始化
     */
    static void pre_initialize();
    /**
     * 主要虚拟机所使用的线程的初始化
     */
    static void global_initialize();
    /**
     * 停止虚拟机所使用的线程
     */
    static void before_destroy();
    /**
     * 进行销毁
     */
    static void destroy();
};

#endif // PLATFORM_MAIN_INIT_HPP
