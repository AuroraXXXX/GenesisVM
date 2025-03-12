//
// Created by aurora on 2025/3/11.
//
#include "metaspace/Metaspace.hpp"
#include "platform/utils/align.hpp"
#include "Setting.hpp"
Mutex Metaspace::Metaspace_lock("Metaspace_lock");
size_t Metaspace::get_meta_bytes_aligned(size_t requested_bytes) {
    requested_bytes = MAX2<size_t>(requested_bytes,metaspace::Setting::MinMetaBytes);
    return align_up(requested_bytes,metaspace::Setting::MetaAlignedBytes);
}
