//
// Created by aurora on 2025/3/12.
//

#ifndef METASPACE_META_LOG_HPP
#define METASPACE_META_LOG_HPP
#include "platform/log.hpp"
namespace metaspace{
#define meta_log(level, message) \
    log_##level(metaspace)(LOG_FMT ":" message,LOG_FMT_ARGS)

#define meta_log2(level, message, ...) \
    log_##level(metaspace)(LOG_FMT ":" message,LOG_FMT_ARGS, __VA_ARGS__)

#define meta_log_stream(level) \
    log_stream(level,metaspace)

}
#endif //METASPACE_META_LOG_HPP
