//
// Created by aurora on 2024/6/29.
//

#ifndef POSEI_INIT_INIT_HPP
#define POSEI_INIT_INIT_HPP

#include "stdtype.hpp"
#include "posei/mem/AllStatic.hpp"

class OSThread;

/**
 * 初始化函数
 * @param vm_start_time vm起始的时间戳
 */
extern posei_init(ticks_t vm_start_time,
                  OSThread *os_thread);
/**
 * 销毁函数 将缓冲区输出
 */
extern posei_destroy();
#endif // POSEI_INIT_INIT_HPP
