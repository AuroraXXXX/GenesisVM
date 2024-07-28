//
// Created by aurora on 2024/7/18.
//

#ifndef KERNEL_CONSTANTS_HPP
#define KERNEL_CONSTANTS_HPP
#include "plat/constants.hpp"
namespace KernelConstants {

    /**
     * 周期性任务 的数量
     */
    constexpr inline uint16_t PeriodicTaskMaxNum = 10;
    /**
     * 周期性任务 检查的单元`
     * Interval 的 1 表示多少毫秒
     */
    constexpr inline uint32_t PeriodicTaskInternalUnit = 10;
    /**
     * 周期性任务 最少的检查的间隔 和最大的检查间隔
     * 间隔的单位是 PeriodicTaskInternalUnitTicks 10ms
     */
    constexpr inline uint32_t PeriodicTaskMinInterval = 1;
    constexpr inline uint32_t PeriodicTaskMaxInterval = 1000;
    /**
     * Arena 清理的 间隔 5000ms
     */
    constexpr inline uint32_t PeriodicTaskArenaClearInterval = 500;
    constexpr inline uint32_t PeriodicNoRunTaskCheckInterval = 10;
}

#endif //KERNEL_CONSTANTS_HPP
