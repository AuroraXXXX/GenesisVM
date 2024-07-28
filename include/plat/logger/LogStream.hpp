//
// Created by aurora on 2024/7/13.
//

#ifndef LOGGING_LOG_STREAM_HPP
#define LOGGING_LOG_STREAM_HPP

#include "plat/stream/CharOStream.hpp"
#include "plat/logger/LogOutput.hpp"
#include "constants.hpp"
#include "LogTagSet.hpp"

class LogStream : public CharOStream {
private:
    /**
     * 辅助类 用于维护行缓冲区
     * 对于较短的行长度，我们避免使用malloc，并使用固定大小的成员char数组。
     * 如果LogStream分配在堆栈上，这意味着小行直接在堆栈上组装。
     */
    class LineBuffer {
    private:
        char _small_buf[64];
        /**
         * 缓冲区的地址
         */
        char *_buf;
        /**
         * 缓冲区的容量
         */
        size_t _cap;
        /**
         * 当前写入的长度
         */
        size_t _pos;

        /**
         * 尝试确保满足需求
         * @param capacity_needed 需求的长度
         */
        void try_ensure_cap(size_t capacity_needed);

    public:
        explicit LineBuffer();

        ~LineBuffer();

        inline char *buf() {
            return this->_buf;
        };

        [[nodiscard]] inline size_t pos() const {
            return this->_pos;
        };

        /**
         * 判断行缓冲区是否 是空的
         * @return
         */
        [[nodiscard]] inline bool is_empty() const { return this->_pos == 0; };

        /**
         * 重置行缓冲区
         */
        void reset();

        /**
         * 向行缓冲区写入数据
         * @param s 数据的首地址
         * @param len 数据的长度
         */
        void append(const char *s, size_t len);
    };

protected:
    /**
     * 用于缓存数据
     */
    LineBuffer _line_buffer;
    /**
     * 用于日志输出等级
     */
    LogLevel _level;
    /**
     * 输出的标签集合
     */
    LogTagSet _tag_set;

    /**
     * 将输出写入到缓冲区的内部
     * @param data
     * @param data_len
     */
    void write(const void *data, size_t data_len) override;

public:
    /**
     * 锁定和解锁 不需要实现
     * 内部是写入到线程独立的缓冲区中
     */
    void lock() override {};

    void unlock() override {};

    /**
     * 刷新内部缓冲区的函数
     * 并且不允许子类去实现
     */
    void flush() final;

    explicit LogStream(LogLevel level,
                       LogTag tag0,
                       LogTag tag1,
                       LogTag tag2,
                       LogTag tag3,
                       LogTag tag4);

    explicit LogStream(LogLevel level,
                       LogTagSet &tag_set);

    ~LogStream() override;

    /**
     * 能否进行日志的输出
     * @return
     */
    [[nodiscard]] inline bool is_enable() const {
        return LogOutput::output_stream()->is_enable(this->_level,
                                                     (LogTagSet *) (&this->_tag_set));
    };

    /**
     * 单条记录实际输出的函数
     * @param level 日志等级
     * @param tag_set 日志标签集合
     * @param format 格式化模板函数
     * @param args 格式化所需参数
     */
    static void record(LogLevel level, LogTagSet &tag_set, const char *format, va_list args);

    /**
     * 格式化字符串
     * @tparam level 日志级别
     * @tparam tag0 标签0
     * @tparam tag1 标签1
     * @tparam tag2 标签2
     * @tparam tag3 标签3
     * @tparam tag4 标签4
     * @param file 文件路径
     * @param line 行号
     * @param format 格式化字符串
     * @param ... 格式化字符串所需的参数
     */
    template<LogLevel level,
            LogTag tag0,
            LogTag tag1,
            LogTag tag2,
            LogTag tag3,
            LogTag tag4>
    static void record(const char *format, ...) {
        LogTagSet logTagSet(tag0, tag1, tag2, tag3, tag4);
        va_list args;
        va_start(args, format);
        LogStream::record(level, logTagSet, format, args);
        va_end(args);
    }

};

#endif //LOGGING_LOG_STREAM_HPP
