//
// Created by aurora on 2022/2/27.
//

#include "plat/logger/LogLayoutFollower.hpp"
#include <cstdio>
#include "plat/os/cpu.hpp"
#include "plat/utils/robust.hpp"
#include "plat/os/time.hpp"
#include "plat/constants.hpp"
#include "plat/logger/log.hpp"
#include "plat/macro.hpp"

static const char *LOG_LEVEL_NAMES[] = {
#define LOG_LEVEL(level) #level ,
        LOG_LEVEL_LIST(LOG_LEVEL)
#undef LOG_LEVEL
};

static int32_t format(char *buf, size_t buf_len, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const auto required_len = ::vsnprintf(buf, buf_len, format, args);
    va_end(args);
    if (required_len < 0 || required_len >= buf_len) {
        assert(required_len < 0, "编码错误");
        return -1;
    }
    return required_len;
}


LogLayoutFollower::LogLayoutFollower(LogTagSet *set, LogLevel level) :
        _level(level),
        _tag_set(set) {
    this->_current_stamp = os::current_stamp();
    this->_elapsed_stamp = os::elapsed_stamp();
    this->_tid = os::current_thread_id();
}


const char *LogLayoutFollower::write_layout_item(LogLayoutType layout,
                                           char *buf,
                                           size_t buf_len) {
    if (layout == LogLayoutType::level) {
        return LOG_LEVEL_NAMES[(uint8_t) this->_level];
    }
    int32_t written;
    switch (layout) {
        case LogLayout::time:
            written = os::iso8061(this->_current_stamp, buf, buf_len, false);
            break;
        case LogLayout::utctime:
            written = os::iso8061(this->_current_stamp, buf, buf_len, true);
            break;
        case LogLayout::uptimes:
            written = format(buf, buf_len, "%.3fs", (double) this->_elapsed_stamp / (double) TicksPerS);
            break;
        case LogLayout::uptimems:
            written = format(buf, buf_len, UINTX_FORMAT "ms", this->_elapsed_stamp / TicksPerMS);
            break;
        case LogLayout::uptimens:
            written = format(buf, buf_len, UINTX_FORMAT "ns", this->_elapsed_stamp / TicksPerNS);
            break;
        case LogLayout::pid:
            written = format(buf, buf_len, INT_FORMAT, os::current_process_id());
            break;
        case LogLayout::tid:
            written = format(buf, buf_len, INT_FORMAT, this->_tid);
            break;
        case LogLayout::tags:
            written = this->_tag_set->write_tags(buf, buf_len);
            break;
        default:
            should_not_reach_here();
            return nullptr;
    }
    return written == -1 ? nullptr : buf;
}


