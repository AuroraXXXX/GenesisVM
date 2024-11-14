//
// Created by aurora on 2024/6/29.
//

#ifndef PLAT_INNER_OS_HPP
#define PLAT_INNER_OS_HPP

#include "stdtype.hpp"
/**
 * 模块内使用的init头文件
 */
namespace os {
    /**
     * 初始化VM启动时间戳和内部的时区
     * @param vm_start_stamp VM启动时间戳
     */
    extern void time_initialize(ticks_t vm_start_stamp);

    /**
     * 用于初始化内部的VM优先级到OS的优先级的映射
     * 不论是否修改这个映射关系，都需要调用这个函数
     * 内部会自行进行判断
     */
    extern void native_prio_initialize();
    /**
     * 当 *uaddr == tag时 挂起线程
     * @param uaddr
     * @param tag 标志 当此值等于uaddr,则进入睡眠
     * @param nsec 超时等待多少ns 0 表示无限期等待
     */
    extern void suspend(const int *uaddr, int tag, uint64_t nsec = 0);
    /**
     * 唤醒等待在uaddr上的num个线程线程
     * @param uaddr int类型变量
     * @param num 线程的数量(如果大于实际等待的线程数量，则唤醒全部线程)
     * @return 返回实际被唤醒的线程数
     */
    extern int wakeup(int *uaddr, int num);
}
#endif //PLAT_INNER_OS_HPP
