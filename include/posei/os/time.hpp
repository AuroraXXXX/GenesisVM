//
// Created by aurora on 2024/6/26.
//

#ifndef PLATFORM_OS_TIME_HPP
#define PLATFORM_OS_TIME_HPP

#include "stdtype.hpp"
#include "plat/constants.hpp"

namespace os {
    extern ticks_t VMStartStamp;


    /**
   * 获取进程的 运行时间
   * @param process_real_time 总的运行时间
   * @param process_user_time 用户态运行时间
   * @param process_system_time 内核态运行时间
   * @return
   */
    bool proc_cpu_time(double &process_real_time,
                       double &process_user_time,
                       double &process_system_time);

    /**
     * 返回距离1970计算机元年的纳秒数
     * @return
     */
    ticks_t current_stamp();

    /**
     * 返回距离启动的纳秒数
     * @return
     */
    inline ticks_t elapsed_stamp(){
        return current_stamp() - VMStartStamp;
    };


    /**
     * 格式化字符串
     * 2023-11-20T00:23:00.000+08:00
     *
     * UTC
     * 2023-11-20T00:23:00.000Z
     * @param current_stamp 时间戳
     * @param buf 缓冲区地址
     * @param buf_len 缓冲区长度 必须大于ISO8601_BUF_SIZE
     * @param utc true  显示0时区时间
     *            false 显示当前时区时间
     * @return -1 表示出现错误
     */
    int32_t iso8061(ticks_t current_stamp,
                     char *buf,
                     size_t buf_len,
                     bool utc,
                     uint8_t decimals = 3);
    /**
     *
     * @param current_stamp
     * @param format 见 strftime 函数
     * @param buf
     * @param buf_len
     * @param utc
     * @return
     */
    OSReturn time_stamp_str(ticks_t current_stamp,
                            const char *format,
                            char *buf,
                            size_t buf_len,
                            bool utc);
}
#endif //PLATFORM_OS_TIME_HPP
