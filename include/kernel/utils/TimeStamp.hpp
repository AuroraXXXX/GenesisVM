//
// Created by aurora on 2022/12/5.
//

#ifndef KERNEL_TIME_STAMP_HPP
#define KERNEL_TIME_STAMP_HPP

#include "plat/constants.hpp"
#include "plat/os/time.hpp"

/**
 * 时间戳
 */
    class TimeStamp {
    private:
        /**
         * 记录纳秒时间戳
         */
        ticks_t _ticks;
    public:
        /**
         * 构造函数
         */
        inline explicit TimeStamp(ticks_t ticks = 0) noexcept: _ticks(ticks) {};

        /**
         * 更新计数器的值
         */
        inline void update_realtime_ticks() {
            this->_ticks = os::current_stamp();
        };

        [[nodiscard]] auto get_sec() const {
            return this->_ticks / TicksPerS;
        };

        /**
         * 获取
         * @return
         */
        [[nodiscard]] auto get_nano_sec() const {
            constexpr auto ns_per_sec = TicksPerS / TicksPerNS;
            return this->_ticks % ns_per_sec;
        };


        /**
         * 返回 计数器的数值
         * @return
         */
        [[nodiscard]] inline auto ticks() const {
            return this->_ticks;
        };

        /**
         * 计算二者的时间的差距
         * @param stamp
         * @return
         */
        TimeStamp during_ns(TimeStamp &stamp) const;

        inline TimeStamp plus(TimeStamp &stamp) const {
            return TimeStamp(this->_ticks + stamp._ticks);
        };

    };



#endif //KERNEL_TIME_STAMP_HPP
