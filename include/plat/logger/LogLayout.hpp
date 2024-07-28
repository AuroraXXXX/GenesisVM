//
// Created by aurora on 2022/2/18.
//

#ifndef LOGGING_LOG_LAYOUT_HPP
#define LOGGING_LOG_LAYOUT_HPP


#include "stdtype.hpp"
/**
 * 关于日志布局器的参数
 * 1 times 表示输出时间 精确到秒
 * 2 utctime 表示输出UTC格式的时间 0时区
 * 3 uptimes 输出距离虚拟机启动的时间 精确到秒
 * 1-4 的选项只能任选其中一个
 * 5 uptimems 输出距离虚拟机启动的时间 精确到毫秒
 * 7 uptimeus 输出距离虚拟机启动的时间 精确到微秒
 * 5-7 的选项只能任选其中一个
 * pid 输出进程号
 * tid 输出线程号
 * level 输出日志消息等级
 * tags 输出日志消息的标签
 */
#define LOG_LAYOUT_PARAM_LIST               \
  LOG_LAYOUT_PARAM(time,         t)       \
  LOG_LAYOUT_PARAM(utctime,       utc)      \
  LOG_LAYOUT_PARAM(uptimes,       us)       \
  LOG_LAYOUT_PARAM(uptimems,      um)      \
  LOG_LAYOUT_PARAM(uptimens,      un)      \
  LOG_LAYOUT_PARAM(pid,           pid)      \
  LOG_LAYOUT_PARAM(tid,           tid)      \
  LOG_LAYOUT_PARAM(level,         l)      \
  LOG_LAYOUT_PARAM(tags,          tg)


class CharOStream;

/**
 * 日志布局器
 *
 * 这些日志布局器应记录某个输出源的日志输出格式
 *
 * 例如，使用'uptime, level, tags'修饰符进行日志记录会导致[0,943s][info][logger]消息。
 *
 */
class LogLayout {
    /**
     * 让日志布局输出器可以访问当前日志布局器的布局信息
     * 用于输出日志
     */
    friend class LogLayoutFollower;

public:
    /**
     * 日志布局器的参数
     */
    enum Type : uint32_t {
#define LOG_LAYOUT_PARAM(name, compress) name,
        LOG_LAYOUT_PARAM_LIST
#undef LOG_LAYOUT_PARAM
        Count,//日志布局参数的个数
        Invalid//表示非法的日志布局参数
    };
private:
    /**
     * 日志布局器中参数各自占用一个比特位 用于表示该参数是否启用
     * 日志布局器中参数的声明顺序 就是比特位占用的顺序
     * 1 表示 启用
     * 0 表示 不启用
     */
    uint32_t _value;
    /**
     * 存储日志布局器参数的名字
     * [...][0]存储日志布局某个参数未压缩的名称
     * [...][1]存储日志布局某个参数已压缩的名称
     */
    static const char *_name[][2];
    /**
     * 默认的日志布局器的布局情况
     */
    static const inline uint32_t DefaultMask = (1u << uptimems) | (1u << level) | (1u << tags);

    /**
     * 获取指定日志布局器参数启用情况的遮罩
     * @param param 日志布局器的某个参数
     * @return
     */
    inline static uint32_t mask(LogLayout::Type param) {
        return 1u << param;
    }

    /**
     * 私有的构造函数 表明在一般情况下不可以构造出来
     * @param value 指定布局器的具体布局信息
     */
    inline  explicit LogLayout(uint32_t value) noexcept: _value(value) {}

public:
    /**
     * 默认已经解析的参数 或者无需解析的
     */
    const static LogLayout Default;

    /**
     * 空的对象
     */
    explicit LogLayout() noexcept: _value(0) {};

    /**
     * 将储存的日志布局信息清空
     */
    inline void clear() { this->_value = 0; };

    /**
     * 获取日志布局某个参数 未压缩的名字
     * @param param 具体的日志局部某个参数
     * @return 相对应的名字
     */
    inline static const char *name(LogLayout::Type param) {
        return _name[param][0];
    };

    /**
     * 获取日志布局某个参数 压缩的名字
     * @param param 具体的日志局部某个参数
     * @return 相对应的名字
     */
    inline static const char *compress_name(LogLayout::Type param) {
        return _name[param][1];
    };

    /**
     * 将字符串转换成对应的某个日志布局参数的枚举值
     * @param str 字符串
     * @return 日志布局参数对应的枚举值
     */
    static LogLayout::Type from_string(const char *str);

    /**
     * 将源日志布局器存储的信息列表合并到本日志布局器中
     * @param source 源日志布局器
     */
    inline void combine_with(LogLayout &source) {
        this->_value |= source._value;
    }

    /**
     * 将指定的日志布局器参数添加到指定的日志布局器中
     * @param target 指定的日志布局器
     * @param d 指定的日志布局器参数
     */
    static inline void add_layout_param(LogLayout &target, LogLayout::Type d) {
        target._value |= LogLayout::mask(d);
    }

    /**
     * 判断本日志布局器信息是否为空
     * @return 空 返回 true
     */
    [[nodiscard]] inline bool is_empty() const {
        return this->_value == 0;
    };

    /**
     * 判断给定的日志布局器参数是否被包含在本日志布局器中
     * @param param 给定的日志布局器参数
     * @return 是否包含在列表中
     */
    [[nodiscard]] inline bool is_contain(LogLayout::Type param) const {
        return (this->_value & LogLayout::mask(param)) != 0;
    };

    /**
     * 自身 与参数layouter 缺少那些参数类型
     * @param layouter
     * @return
     */
    inline LogLayout lack_for(LogLayout &layouter) const {
        return LogLayout((~this->_value) & (layouter._value));
    };

    /**
     * 解析配置的字符串 不同的日志解析器参数之间使用,号进行分割
     * @param params_args 有关日志布局信息的字符串 不区分大小写
     *      .nullptr或者"none"表示日志的布局信息是空
     * @param err_stream 错误的输出流
     * @return 是否解析成功
     */
    bool parse(const char *params_args,CharOStream *err_stream);
};

typedef LogLayout::Type LogLayoutType;

#endif //LOGGING_LOG_LAYOUT_HPP
