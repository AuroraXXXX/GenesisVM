//
// Created by aurora on 2023/12/3.
//

#include "plat/stream/OStream.hpp"


OStream::OStream() :
        _statistics_bytes(0),
        _ticks(0) {}


void OStream::write_bytes(void *addr, size_t bytes) {
    this->write(addr, bytes);
    this->_statistics_bytes += bytes;
}
