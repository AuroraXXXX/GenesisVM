//
// Created by aurora on 2024/7/13.
//

#include "plat/logger/LogSingleFileOutput.hpp"
#include "plat/utils/robust.hpp"
#include "plat/stream/CharOStream.hpp"

bool LogSingleFileOutput::is_enable(LogLevel level, LogTagSet *tag_set) {
    return (uint8_t) level >= (uint8_t) this->_level;
}

void LogSingleFileOutput::write(LogLayoutFollower *follower, char *data, size_t data_len) {
    const auto stream = this->_stream;
    stream->lock();
    LogOutput::write_file_format_follow(stream,
                                        follower,
                                        this->_layout,
                                        this->_layout_padding);
    stream->print_raw("\t");
    stream->print(data, data_len);
    stream->unlock();
}

LogSingleFileOutput::LogSingleFileOutput(LogLevel level,
                                         LogLayout layout,
                                         CharOStream *stream) :
        _stream(stream),
        _level(level),
        _layout(layout) {
    assert(stream != nullptr, "stream is null");
    for (auto &item: this->_layout_padding) {
        item = 0;
    }
}

LogSingleFileOutput::~LogSingleFileOutput() {
    this->_stream->flush();
}
