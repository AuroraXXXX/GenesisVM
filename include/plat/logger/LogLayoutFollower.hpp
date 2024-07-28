//
// Created by aurora on 2022/2/27.
//

#ifndef LOGGING_LOG_LAYOUT_FOLLOWER_HPP
#define LOGGING_LOG_LAYOUT_FOLLOWER_HPP

#include "plat/mem/allocation.hpp"
#include "plat/logger/constants.hpp"
#include "plat/logger/LogLayout.hpp"

class LogTagSet;

/**
 * 日志布局跟随器
 *
 * 存储有关日志布局的临时对象
 *
 * 每一条日志消息仅仅对应于 一个日志布局
 * 会按照不同的布局器进行个性化输出
 *
 * 同时也能保证向多个输出源 输出的时间等信息一致性
 */
class LogLayoutFollower : public StackObject {
private:
    LogLevel _level;
    int32_t _tid;
    /**
     * 时间戳
     * 距离1970的秒数 和多余的纳秒
     */
    ticks_t _current_stamp;
    ticks_t _elapsed_stamp;
    /**
     * 表示需要输出的标签集合
     */
    LogTagSet *_tag_set;
public:
    /**
     * 构造函数
     * @param set
     * @param level
     */
    explicit LogLayoutFollower(LogTagSet *set, LogLevel level);

    /**
     * 写出一个布局参数
     * @param layout
     * @param buf
     * @param buf_len
     * @return nullptr 编码错误或者缓冲区太小
     */
    const char *write_layout_item(LogLayoutType layout,
                                  char *buf,
                                  size_t buf_len);
};


#endif //LOGGING_LOG_LAYOUT_FOLLOWER_HPP
