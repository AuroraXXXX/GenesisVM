//
// Created by aurora on 2025/3/25.
//

#include "RootArea.hpp"
#include "new"
#include "platform/allocation.hpp"
#include "Segment.hpp"

metaspace::RootArea::RootArea(metaspace::Volume *volume, uint32_t total) :
        _top(0),
        _total(total),
        _volume(volume) {

}

metaspace::RootArea *metaspace::RootArea::create(metaspace::Volume *volume, uint32_t len) {
    const size_t total_bytes = sizeof(RootArea) + sizeof(LinkList<Segment>) * len;
    const auto mem_ptr = NEW_CHEAP_ARRAY(char, total_bytes, MEMFLAG::Metaspace);
    return new(mem_ptr)RootArea(volume, len);
}
