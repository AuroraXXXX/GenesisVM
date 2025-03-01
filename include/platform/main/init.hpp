//
// Created by aurora on 2024/6/29.
//

#ifndef PLATFORM_MAIN_INIT_HPP
#define PLATFORM_MAIN_INIT_HPP

#include "platform/typedef.hpp"

class OSThread;

/**
 * 初始化函数
 */
extern void platform_init();
/**
 * 销毁函数 将缓冲区输出
 */
extern void  platform_destroy();
#endif // PLATFORM_MAIN_INIT_HPP
