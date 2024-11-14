//
// Created by aurora on 2024/7/13.
//

#ifndef LOGGING_LOG_FILE_OUTPUT_HPP
#define LOGGING_LOG_FILE_OUTPUT_HPP

#include "plat/logger/LogOutput.hpp"
/**
 * 文件格式的日志输出流
 * 是单个文件的输出
 */
class LogSingleFileOutput : public LogOutput {
    /**
     * 日志的等级
     * 高于或者等于此日志等级的消息才会被输出
     */
    LogLevel _level;
    /**
     * 日志布局器
     * 就是决定输出那些日志布局参数
     */
    LogLayout _layout;
    uint32_t _layout_padding[LogLayoutType::Count];

    CharOStream *_stream;

public:
    /**
     * 构造函数
     * @param level 日志输出的等级
     * @param layout 布局参数
     * @param stream 输出流对象
     */
    explicit LogSingleFileOutput(LogLevel level,
                                 LogLayout layout,
                                 CharOStream *stream);

    ~LogSingleFileOutput() override;

    bool is_enable(LogLevel level, LogTagSet *tagSet) override;

    void write(LogLayoutFollower *follower, char *data, size_t data_len) override;

};


#endif //LOGGING_LOG_FILE_OUTPUT_HPP
