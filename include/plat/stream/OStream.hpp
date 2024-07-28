//
// Created by aurora on 2023/12/3.
//

#ifndef PLATFORM_OSTREAM_HPP
#define PLATFORM_OSTREAM_HPP

#include "stdtype.hpp"
#include <endian.h>

/**
 * 写出到输出流 并自动转换成网络字节序
 */
class OStream {
private:

    /**
     * 输出的时间戳
     */
    ticks_t _ticks;
    /**
     * 实际写出的字符数
     */
    size_t _statistics_bytes;
protected:


    [[nodiscard]] inline auto ticks() const {
        return this->_ticks;
    };

    /**
     * 将数据写出实际的目的地
     * 底层应该保证全部写出 否则应该给出错误 中止VM等
     * @param data 数据的首地址
     * @param data_len 数据的长度
     * @return 实际写入的字节数
     */
    virtual void write(const void *data, size_t data_len) = 0;


public:
    explicit OStream();

    virtual ~OStream() = default;

    /**
     * 锁定文件
     */
    virtual void lock() = 0;

    /**
     * 解除文件的锁定
     */
    virtual void unlock() = 0;

    /**
     * 将数据从缓冲区刷新到实际的地方
     */
    virtual void flush() {};

    /**
     * 输出8字节的时间戳
     * 到输出流中
     */
    inline void ns_stamp() {
        this->write_uint64(this->_ticks);
    };

    [[nodiscard]] inline auto statistics_bytes() const {
        return this->_statistics_bytes;
    };

    /**
     * 写出1字节的数据
     * @param value 数据
     */
    inline void write_uint8(uint8_t value) {
        this->write_bytes(&value, sizeof(uint8_t));
    };

    /**
     * 写出2字节的数据
     * @param value 数据
     */
    inline void write_uint16(uint16_t value) {
        value = htobe16(value);
        this->write_bytes(&value, sizeof(value));
    };

    /**
     * 写出4字节的数据
     * @param value 数据
     */
    inline void write_uint32(uint32_t value) {
        value = htobe32(value);
        this->write_bytes(&value, sizeof(value));
    };

    /**
     * 写出8字节的数据
     * @param value 数据
     */
    inline void write_uint64(uint64_t value) {
        value = htobe64(value);
        this->write_bytes(&value, sizeof(value));
    };

    /**
     * 外界包括子类写入数据，实际上需要调用这个函数
     * 因为这个函数 会修改统计
     * @param addr 数据的首地址
     * @param bytes 数据的长度
     */
    void write_bytes(void *addr, size_t bytes);
};


#endif //PLATFORM_OSTREAM_HPP
