//
// Created by aurora on 2023/12/21.
//
#include "plat/os/time.hpp"
#include <unistd.h>
#include <sys/times.h>
#include <cstdio>
#include <ctime>
#include "plat/constants.hpp"
#include "plat/utils/robust.hpp"
namespace os {
    /**
     * 用于表示时区的
     */
    static char TimeZoneBuf[6] = {0};

    /**
     * VM启动的时间戳
     */
    ticks_t VMStartStamp = 0;

    /**
     * 用于格式化时区信息
     *
     * 在头文件中定义timezone表示的是
     * 当前时区距离格林威治偏移的秒数
     * 美洲 >0 大部分 欧洲 亚洲 非洲 <0
     *
     * 但是正常时区显示 东区 显示 +
     */
    void time_initialize(ticks_t vm_start_stamp) {
        //我们仅仅精确到分钟 所以除去60 由于东时区是 + 号，所以还要取反
        long local_utc = -timezone;
        TimeZoneBuf[0] = local_utc > 0 ? '+' : '-';
        constexpr int seconds_per_hour = SPerMin * MinPerHour;
        const long zone_hours = local_utc / seconds_per_hour;
        local_utc -= zone_hours * seconds_per_hour;
        const long zone_mins = local_utc / SPerMin;
        ::snprintf(TimeZoneBuf + 1,
                   sizeof(TimeZoneBuf) - 1,
                   "%02d%02d",
                   (int) zone_hours,
                   (int) zone_mins);
        //记录虚拟机启动时间
        VMStartStamp = vm_start_stamp;
    }

    bool proc_cpu_time(double &process_real_time,
                       double &process_user_time,
                       double &process_system_time) {
        struct tms ticks{};
        //获取滴答数
        clock_t real_ticks = times(&ticks);
        //出现错误
        if (real_ticks == (clock_t) (-1)) {
            return false;
        }

        static auto tics_per_sec = (int32_t) ::sysconf(_SC_CLK_TCK);
        process_user_time = ((double) ticks.tms_utime) / tics_per_sec;
        process_system_time = ((double) ticks.tms_stime) / tics_per_sec;
        process_real_time = ((double) real_ticks) / tics_per_sec;
        return true;
    }


    ticks_t current_stamp() {
        struct ::timespec spec{};
        clock_gettime(CLOCK_REALTIME, &spec);
        //对时间戳进行规范化处理
        constexpr auto ns_per_sec = TicksPerS / TicksPerNS;
        while (spec.tv_nsec > ns_per_sec) {
            spec.tv_nsec -= ns_per_sec;
            spec.tv_sec++;
        }
        while (spec.tv_nsec < 0) {
            spec.tv_nsec += ns_per_sec;
            spec.tv_sec--;
        }
        return (spec.tv_sec * ns_per_sec + spec.tv_nsec);
    }


    int32_t iso8061(
            ticks_t current_stamp,
            char *buf,
            size_t buf_len,
            bool utc,
            uint8_t decimals) {
        if (buf_len < ISO8601_BUF_SIZE || buf == nullptr) {
            assert(false,"null buf or buf_len is too small");
            return -1;
        }
        //获取时间戳对应的 秒数 和 毫秒数
        constexpr auto ns_per_sec = TicksPerS / TicksPerNS;
        const auto seconds_since_19700101 = (long) (current_stamp / ns_per_sec);
        const auto nanos_after_19700101 = (long) (current_stamp - seconds_since_19700101 * ns_per_sec);
        struct tm time_struct{};
        if (utc) {
            /**
             * 这个函数获取的时间是UTC时间，
             * 并且使用以r结尾的函数是线程安全的
             */
            auto p = gmtime_r(&seconds_since_19700101, &time_struct);
            if (p == nullptr){
                assert(false, "Failed gmtime_r");
                return -1;
            }
        } else {
            /**
             * 这个函数获取的时间是当前时区的
             * 并且使用以r结尾的函数是线程安全的
             */
            auto p = localtime_r(&seconds_since_19700101, &time_struct);
            if (p == nullptr) {
                assert(false, "Failed localtime_r");
                return -1;
            }
        }
        const auto zone_buf = utc ? "+0000" : TimeZoneBuf;
        const auto decimal_value = (double) nanos_after_19700101 /
                                   (double) ns_per_sec + time_struct.tm_sec;
        auto printLen = ::snprintf(buf, buf_len,
                                   "%04d-%02d-%02dT%02d:%02d:%0*.*f%s",
                                   1900 + time_struct.tm_year,
                                   1 + time_struct.tm_mon,
                                   time_struct.tm_mday,
                                   time_struct.tm_hour,
                                   time_struct.tm_min,
                                   (decimals + 3),
                                   decimals,
                                   decimal_value,
                                   zone_buf);
        if(printLen == 0){
            return -1;
        }else{
            return printLen;
        }
    }

    OSReturn time_stamp_str(
            ticks_t current_stamp,
            const char *format,
            char *buf,
            size_t buf_len,
            bool utc) {
        assert(buf != nullptr, "buf must not null");
        if (buf == nullptr) {
            return OSReturn::NO_MEMORY;
        }
        //获取时间戳对应的 秒数 和 毫秒数
        constexpr auto ns_per_sec = TicksPerS / TicksPerNS;

        const long seconds_since_19700101 = (long) current_stamp / ns_per_sec;
        struct tm time_struct{};
        if (utc) {
            /**
             * 这个函数获取的时间是UTC时间，
             * 并且使用以r结尾的函数是线程安全的
             */
            auto p = gmtime_r(&seconds_since_19700101, &time_struct);
            if (p == nullptr) return OSReturn::ERR;
        } else {
            /**
             * 这个函数获取的时间是当前时区的
             * 并且使用以r结尾的函数是线程安全的
             */
            auto p = localtime_r(&seconds_since_19700101, &time_struct);
            if (p == nullptr) return OSReturn::ERR;
        }
        time_struct.tm_gmtoff = -timezone;
        auto res = ::strftime(buf, buf_len, format, &time_struct);
        return res != 0 ? OSReturn::OK : OSReturn::NO_MEMORY;
    }
}
