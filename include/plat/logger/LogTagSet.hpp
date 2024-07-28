//
// Created by aurora on 2022/10/17.
//

#ifndef LOGGING_LOG_TAG_SET_HPP
#define LOGGING_LOG_TAG_SET_HPP

#include "constants.hpp"
#include <cstdarg>
#include "plat/utils/robust.hpp"

/**
 * 记录日志的集合
 */
class LogTagSet {
    friend class PlatInitialize;
private:
    /**
     * 标签的集合
     */
    LogTag _tags[LogTagSetMax];

    explicit LogTagSet() noexcept;
    /**
     * 前端传入标签名称
     */
    static const char **_tags_name;
    static LogTag _tags_max;

public:
    const static LogTagSet Default;

    /**
     * 标签集合字符串
     * 全部设置默认空标签
     * @param tag0
     * @param tag1
     * @param tag2
     * @param tag3
     * @param tag4
     */
    explicit LogTagSet(
            LogTag tag0,
            LogTag tag1,
            LogTag tag2,
            LogTag tag3,
            LogTag tag4) noexcept;

    /**
     * 格式化输出标签集合字符串
     * @param buf 缓冲区
     * @param buf_len 缓冲区的长度
     * @param split 标签的分割符
     *         -1 表示出现错误或者缓冲区错误
     */
    int write_tags(char *buf, size_t buf_len, const char *split = ",");
};


#endif //LOGGING_LOG_TAG_SET_HPP
