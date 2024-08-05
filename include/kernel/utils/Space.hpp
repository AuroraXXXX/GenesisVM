//
// Created by aurora on 2022/12/9.
//

#ifndef VMACHINE_MEMREGION_HPP
#define VMACHINE_MEMREGION_HPP

#include "stdtype.hpp"

/**
 * 表示一段内存空间
 */
class Space {
protected:
    uintptr_t _start;
    uintptr_t _end;

    inline Space(void *start, void *end) noexcept:
            _start((uintptr_t) start),
            _end((uintptr_t) end) {
    }

public:
    explicit Space() noexcept:
            _start(0),
            _end(0) {};

    explicit Space(void *start, size_t length) :
            _start((uintptr_t) start),
            _end((uintptr_t) start + length) {};

    /**
     * 内存区域的起始地址
     * @return
     */
    [[nodiscard]] inline auto start() const {
        return (void *) (this->_start);
    };

    [[nodiscard]] inline auto start_literal() const {
        return this->_start;
    };

    /**
     * 内存区域的结束地址 ，不包含
     * @return
     */
    [[nodiscard]] inline auto end() const {
        return (void *) (this->_end);
    };

    [[nodiscard]] inline auto end_literal() const {
        return this->_end;
    };

    /**
     * 最后一字节 有效的内存地址
     * @return
     */
    [[nodiscard]] inline auto last() const {
        return (void *) (this->_end - 1);
    };

    [[nodiscard]] inline auto last_literal() const {
        return this->_end - 1;
    };


    /**
     * 内存区域表示的长度
     * @return
     */
    [[nodiscard]] inline auto capacity_bytes() const {
        return this->_end - this->_start;
    };

    /**
     * 是否包含 参数指定的内存区域region
     * @param region
     * @return
     */
    [[nodiscard]] inline bool contains(Space region) const {
        return this->_start <= region._start && this->_end >= region._end;
    };

    /**
     * 内存区域是否包含指针
     * @param addr
     * @return
     */
    inline bool contains(const void *addr) const {
        return (uintptr_t) addr >= this->_start && (uintptr_t) addr <= this->_end;
    };

    /**
     * 判断是否是空
     * @return
     */
    [[nodiscard]] inline bool is_empty() const {
        return this->_start == this->_end;
    };

    /**
     * 判断两个内存区域是否相等
     * @param region
     * @return true 表示相等
     *          false 表示不相等
     */
    [[nodiscard]] inline bool equals(Space region) const {
        return (region.is_empty() && this->is_empty()) ||
               (this->_start == region._start && this->_end == region._end);
    };

    /**
     * 交集
     * 两块内存区域重叠部分
     * @param region 另外一块内存区域
     * @return
     */
    [[nodiscard]] Space intersection(Space region) const;

    /**
     * 差集
     * 本区域内与 参数region指定的区域 不相交的区域
     * @param region
     * @return
     */
    [[nodiscard]] Space difference(Space region) const;

    /**
     * 并集
     * 将两个区域合并成一个区域
     * @param region
     * @return
     */
    [[nodiscard]] Space _union(Space region) const;
};


#endif //VMACHINE_MEMREGION_HPP
