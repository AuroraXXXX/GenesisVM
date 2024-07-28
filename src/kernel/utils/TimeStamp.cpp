//
// Created by aurora on 2022/12/5.
//

#include "kernel/utils/TimeStamp.hpp"

TimeStamp TimeStamp::during_ns(TimeStamp &stamp) const {
    auto during = stamp._ticks - this->_ticks;
    return TimeStamp(during > 0 ? during : -during);
}




