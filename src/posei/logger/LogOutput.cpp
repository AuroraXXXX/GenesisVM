//
// Created by aurora on 2024/7/13.
//

#include "plat/logger/LogOutput.hpp"
#include "plat/logger/LogLayout.hpp"
#include "plat/logger/LogLayoutFollower.hpp"
#include "plat/utils/robust.hpp"
#include "plat/logger/LogSingleFileOutput.hpp"
#include "plat/stream/FileCharOStream.hpp"
#include "plat/utils/OrderAccess.hpp"
LogOutput *volatile LogOutput::_stream = nullptr;
static LogSingleFileOutput DEFAULT(LogLevel::default_console_level,
                                   LogLayout::Default,
                                   FileCharOStream::default_stream());

void LogOutput::write_file_format_follow(
        CharOStream *stream,
        LogLayoutFollower *follower,
        LogLayout layout,
        uint32_t *layout_padding) {
    char buf[LOG_MAX_FOLLOWER_SIZE];
    for (uint32_t i = 0; i < LogLayout::Count; ++i) {
        const auto type = LogLayout::Type(i);
        if (!layout.is_contain(type)) {
            continue;
        }
        auto write_buf = follower->write_layout_item(type, buf, sizeof(buf));
        if (write_buf == nullptr) {
            assert(false, "buf is small or encode is error");
            return;
        }
        const auto padding = layout_padding[i];
        const auto before = stream->statistics_bytes();
        const auto return_status = stream->print("[%-*s]",
                                                 padding,
                                                 write_buf);
        const auto after = stream->statistics_bytes();
        if (return_status != OSReturn::OK || before == after) {
            return;
        }
        const auto written = (int32_t) (after - before);
        const auto value_written = written - 2;
        if (value_written > padding) {
            layout_padding[i] = value_written;
        }
    }
}

LogOutput *LogOutput::output_stream() {
    const auto stream = OrderAccess::load(&LogOutput::_stream);
    if (stream == nullptr) {
        return &DEFAULT;
    } else {
        return stream;
    }

}

void LogOutput::register_global(LogOutput *stream) {

    OrderAccess::store(&LogOutput::_stream, stream);
}

