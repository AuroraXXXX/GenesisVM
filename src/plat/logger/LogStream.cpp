//
// Created by aurora on 2024/7/2.
//
#include "plat/utils/robust.hpp"
#include "plat/utils/align.hpp"
#include "plat/mem/allocation.hpp"
#include "plat/stream/FileCharOStream.hpp"
#include "plat/logger/LogLayoutFollower.hpp"
#include "plat/logger/LogOutput.hpp"
#include "plat/logger/LogStream.hpp"
#include "plat/thread/OSThread.hpp"


void LogStream::record(LogLevel level, LogTagSet &tag_set, const char *format, va_list args) {
    ResourceArenaMark mark;
    LogStream stream(level, tag_set);
    stream.do_vsnprintf_with_buf(true, format, args);
}


void LogStream::write(const void *data, size_t data_len) {
    if (!this->is_enable()) {
        return;
    }
    auto data_str = (const char *) data;
    if (data_len > 0) {
        this->_line_buffer.append(data_str, data_len);
    }
}

LogStream::LogStream(LogLevel level,
                     LogTag tag0,
                     LogTag tag1,
                     LogTag tag2,
                     LogTag tag3,
                     LogTag tag4) :
        _line_buffer(),
        _level(level),
        _tag_set(tag0, tag1, tag2, tag3, tag4) {
}

LogStream::LogStream(LogLevel level, LogTagSet &tag_set) :
        _level(level),
        _line_buffer(),
        _tag_set(tag_set) {

}

LogStream::~LogStream() {
    this->flush();
}


void LogStream::flush() {
    auto &line_buffer = this->_line_buffer;
    if (line_buffer.is_empty() || !this->is_enable()) {
        return;
    }
    LogLayoutFollower follower(&this->_tag_set, this->_level);
    //进行数据输出
    auto output = LogOutput::output_stream();
    const auto data = line_buffer.buf();
    const auto data_len = line_buffer.pos();
    /**
     * 当没有初始化输出函数的时候
     * 使用标准输出
     */
    output->write(&follower,data,data_len);
    line_buffer.reset();
}





/***
 * --------------------
 *
 * --------------------
 */
void LogStream::LineBuffer::try_ensure_cap(size_t capacity_needed) {
    /**
        * 容量最小是_small_buf 不足会申请更大的
        */
    constexpr auto SMALL_BUF_SIZE = sizeof(this->_small_buf);
    assert(this->_cap >= SMALL_BUF_SIZE, "程序错误");
    //满足就直接返回即可
    if (this->_cap >= capacity_needed) {
        return;
    }
    assert(this->_cap <= OStreamDefaultBufSize, "程序错误");
    constexpr size_t addition_expansion = 256;
    size_t new_cap = align_up(capacity_needed + addition_expansion, addition_expansion);
    new_cap = MIN2<size_t>(OStreamDefaultBufSize, new_cap);
    auto new_buf = (char *) NEW_RESOURCE_ARRAY(char, new_cap);
    if (new_buf == nullptr) {
        return;
    }
    if (this->_pos > 0) {
        //还要+1包括结束符
        ::memcpy(new_buf, this->_buf, this->_pos + 1);
    }
    if (this->_buf != this->_small_buf) {
        //说明原本的内存块也是malloc申请的
        FREE_RESOURCE_ARRAY(this->_buf, this->_cap);
    }
    this->_buf = new_buf;
    this->_cap = new_cap;
    assert(this->_cap > capacity_needed, "程序错误");
}

LogStream::LineBuffer::LineBuffer() :
        _buf(_small_buf),
        _cap(sizeof(_small_buf)),
        _pos(0) {
    _small_buf[0] = '\0';
}

LogStream::LineBuffer::~LineBuffer() {
    assert(this->_pos == 0, "日志未输出完毕");
    if (this->_buf != this->_small_buf) {
        // os::free(this->_buf, MEMFLAG::Logging, CALLER_STACK);
        FREE_RESOURCE_ARRAY(this->_buf, this->_cap);
    }
}

void LogStream::LineBuffer::reset() {
    this->_pos = 0;
    this->_buf[0] = '\0';
}

void LogStream::LineBuffer::append(const char *s, size_t len) {
    assert(this->_buf[this->_pos] == '\0', "程序错误");
    assert(this->_pos < this->_cap, "程序错误");
    //需要最小的长度
    const auto minni_capacity_need = this->_pos + len + 1;
    this->try_ensure_cap(minni_capacity_need);
    if (this->_cap < minni_capacity_need) {
        len = this->_cap - this->_pos - 1;
        if (len == 0) {
            return;
        }
    }
    ::memcpy(this->_buf + this->_pos, s, len);
    this->_pos += len;
    this->_buf[this->_pos] = '\0';
}
