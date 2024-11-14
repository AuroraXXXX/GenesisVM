//
// Created by aurora on 2024/7/9.
//

#ifndef LOGGING_LOG_OUTPUT_CALLBACK_HPP
#define LOGGING_LOG_OUTPUT_CALLBACK_HPP

#include "constants.hpp"
#include "plat/logger/LogLayout.hpp"


class LogLayoutFollower;

class LogTagSet;

/**
 * 日志输出函数
 */
class LogOutput {
    friend class LogStream;

private:
    static LogOutput *volatile _stream;

    /**
     * 获取日志的输出流
     * @return
     */
    static  LogOutput *output_stream();

protected:
    /**
     * 将日志的头部标使用文件格式进行输出
     * [xxx][xxxx]... [xxx]
     * @param stream 日志的具体输出流
     * @param follower 存储日志信息的日志跟随器
     * @param layout 日志前缀的格式
     * @param layout_padding 日志前缀各项的填充，用于多条日志记录之间前缀进行对齐
     */
    static void write_file_format_follow(CharOStream *stream,
                                         LogLayoutFollower *follower,
                                         LogLayout layout,
                                         uint32_t *layout_padding);

public:
    /**
     * 判断能否进行输出
     * @param level 输出的日志等级
     * @param tag_set 输出的标签集合
     * @return 能否进行日志的输出
     */
    virtual bool is_enable(LogLevel level, LogTagSet *tag_set) = 0;

    /**
     * 将整个日志数据进行输出 本方法应该自行保证原子性
     * @param follower 某条日志存储的相关信息
     * @param data 日志主题部分数据的首地址
     * @param data_len 日志主题部分数据的长度
     */
    virtual void write(LogLayoutFollower *follower, char *data, size_t data_len) = 0;

    /**
     * 析构函数
     */
    virtual  ~LogOutput() = default;

    /**
     * 注册全局的日志输出流
     * @param stream 全局的日志输出流
     */
    static inline void register_global(LogOutput *stream);

};


#endif //LOGGING_LOG_OUTPUT_CALLBACK_HPP
