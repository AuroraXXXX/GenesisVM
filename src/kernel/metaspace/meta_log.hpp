//
// Created by aurora on 2024/8/12.
//

#ifndef GENESISVM_META_LOG_HPP
#define GENESISVM_META_LOG_HPP
#include "plat/logger/log.hpp"
namespace kernel{
#define meta_log(level, message) \
    log_##level(metaspace)(LOG_FMT ":" message,LOG_FMT_ARGS)

#define meta_log2(level, message, ...) \
    log_##level(metaspace)(LOG_FMT ":" message,LOG_FMT_ARGS, __VA_ARGS__)

#define meta_log_stream(level) \
    log_stream(level,metaspace)

}
#endif //GENESISVM_META_LOG_HPP
