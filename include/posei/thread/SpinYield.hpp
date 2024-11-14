//
// Created by aurora on 2023/1/22.
//

#ifndef PLAT_THREAD_SPIN_YIELD_HPP
#define PLAT_THREAD_SPIN_YIELD_HPP

#include "plat/mem/allocation.hpp"
#include "plat/constants.hpp"

/**
 * 自旋或者睡眠
 */
class SpinYield : public StackObject {
private:
    /**
     * 睡眠的纳秒数
     */
    ticks_t _sleep_ticks;
    /**
     * 已经自旋的次数
     */
    uint32_t _spins;
    /**
     * 已经让出CPU的次数
     */
    uint32_t _yields;
    /**
     * 约束的条件
     */
    const uint32_t _spin_limit;
    const uint32_t _yield_limit;
    /**
     * 睡眠的一次的纳秒数 最多1s
     */
    const uint32_t _per_sleep_ns;

    /**
     * 让出CPU或者睡眠
     * 取决于 _yield_limit
     */
    void yield_or_sleep();

public:


    /**
     * 这个函数强制不内联 用于自旋
     * @return
     */
    static ALWAYS_NOT_INLINE int spin_pause();

    /**
     * 短暂的时间暂停和休眠
     * 演变的进程 SPIN -> YIELD -> SLEEP
     * @param spin_limit 自旋的限制 仅仅在多核CPU才有效
     *                      0 表示 不自旋
     * @param yield_limit 让出CPU的限制
     *                      0 表示不让出 CPU
     * @param sleep_ns 睡眠的时间(每次)
     */
    explicit SpinYield(uint32_t spin_limit = SpinDefaultSpinLimit,
                       uint32_t yield_limit = SpinDefaultYieldLimit,
                       uint32_t per_sleep_ns = SpinDefaultSleepNs);

    /**
     * 等待一次
     */
    void wait();

    /**
     * 睡眠 最多1s
     * @param ns 睡眠的时间
     * @return 实际的睡眠时间
     */
    static ticks_t sleep(uint32_t ns);

    /**
     * 报告信息
     * @param out
     */
    void report(CharOStream *out) const;
};


#endif //PLAT_THREAD_SPIN_YIELD_HPP
