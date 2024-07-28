//
// Created by aurora on 2022/1/17.
//

#ifndef LOGGING_LOG_HPP
#define LOGGING_LOG_HPP

#include "constants.hpp"
#include <cstdarg>
#include "stdtype.hpp"
#include "LogStream.hpp"

/**
 * 一行日志主要包括
 * 日志的模板 日志的前缀 日志的实际数据
 * 1.日志的模板 可以指定 example: [0.256s][debug]
 * 2.日志的前缀 GC线程在完成GC后会专门输出一个前缀 example: GC(1078)
 *            其他的情况日志的前缀是 ""
 */




#define LOG_TAGS_EXPANDED(T0, T1, T2, T3, T4, ...)  \
                                    PREFIX_LOG_TAG(T0), \
                                    PREFIX_LOG_TAG(T1), \
                                    PREFIX_LOG_TAG(T2), \
                                    PREFIX_LOG_TAG(T3), \
                                    PREFIX_LOG_TAG(T4)

/**
 * , no_tag, no_tag, no_tag, no_tag, no_tag, no_tag
 * 添加5个no_tag，用于自动补充
 */
#define LOG_TAGS(args...) LOG_TAGS_EXPANDED(args,no_tag, no_tag, no_tag, no_tag, no_tag)
/**
 * 只有这一条日志的等级大于或者等于 监听的等级 这条日志才会写入到 日志文件中
 * 日志的级别: [trace]<[debug]<[info]<[warn]<[error]<[off]
 */
#define log_trace(ARGS...) LogStream::record<LogLevel::trace,LOG_TAGS(ARGS)>
#define log_debug(ARGS...) LogStream::record<LogLevel::debug,LOG_TAGS(ARGS)>
#define log_info(ARGS...) LogStream::record<LogLevel::info,LOG_TAGS(ARGS)>
#define log_warn(ARGS...) LogStream::record<LogLevel::warn,LOG_TAGS(ARGS)>
#define log_error(ARGS...) LogStream::record<LogLevel::error,LOG_TAGS(ARGS)>
/**
 * 使用日志输出流
 */
#define log_stream(level,ARGS...) LogStream log(LogLevel::level,LOG_TAGS(ARGS));

#endif //LOGGING_LOG_HPP
