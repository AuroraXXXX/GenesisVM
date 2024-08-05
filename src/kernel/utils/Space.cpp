//
// Created by aurora on 2022/12/9.
//

#include "kernel/utils/Space.hpp"
#include "plat/macro.hpp"
#include "plat/utils/robust.hpp"
Space Space::intersection(Space region) const {
    auto start = MAX2(this->start(), region.start());
    auto end = MIN2(this->end(), region.end());
    Space result;
    if (start < end) {
        result._start = (uintptr_t) start;
        result._end = (uintptr_t) end;
    }
    return result;
}

Space Space::difference(Space region) const {
    /**
     *               [ 本 内存区域  ]
     * [  case1  ]
     *       [  case2  ]
     *                 [ case3 ]
     *                      [  case4  ]
     *                                  [  case5  ]
     *        [         case6          ]
     */
    if (region.end() <= this->start()) {
        //case1
        return *this;
    }
    if (region.start() <= this->start() && region.end() <= this->end()) {
        //case2
        return {region.end(), this->end()};
    }
    if (region.start() >= this->start() && region.end() >= this->end()) {
        //case4
        return {this->start(), region.start()};
    }
    if (region.start() >= this->end()) {
        //case5
        return *this;
    }
    if (region.start() <= this->start() && region.end() >= this->end()) {
        //case6
        return Space();
    }
    //case4
    guarantee(false, "Space:minus:内部,不存在这样的差集.");
    return Space();
}

Space Space::_union(Space region) const {
    if (this->is_empty()) return region;
    if (region.is_empty()) return *this;
    //应该是重叠或者包含
    assert((this->start() <= region.start() && this->end() >= region.start()) ||
           (region.start() <= this->start() && region.end() >= this->start()),
           "没有重叠和包含的区域"
    );
    auto start = MIN2(this->start(), region.start());
    auto end = MAX2(this->end(), region.end());
    return Space(start, end);
}


