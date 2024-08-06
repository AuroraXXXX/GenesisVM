//
// Created by xu on 2023/8/26.
//

#ifndef PLATFORM_CHAR_OSTREAM_HPP
#define PLATFORM_CHAR_OSTREAM_HPP

#include "OStream.hpp"
#include <cstring>
#include <cstdarg>
#include "plat/constants.hpp"

/**
 * 在二进制输出流基础上实现对字符流的输出
 *
 * 直接输出字符串是不会发生截断情况
 * 但是 如果格式化的字符串超过 O_BUFLEN
 * 会调用 alloc_internal_buf 申请内存
 * 如果获取到满足要求的内存会再次进行格式化
 * 并进行输出
 * 如果没有获取足够的内存，即函数返回null
 * 会输出使用不超过 O_BUFLEN 的截断的字符串
 * 并且也不会调用 dealloc_internal_buf
 *
 */
class CharOStream : public OStream {
protected:

    /**
     * 使用缓冲区进行格式化
     * @param cr 是否进行换行
     * @param format 进行格式化模板
     * @param ap 参数
     * @return 结果
     */
    OSReturn do_vsnprintf_with_buf(bool cr,
                                   const char *format,
                                   va_list ap);


    virtual void *alloc_internal_buf(int32_t buf_len) {
        return nullptr;
    };

    virtual void free_internal_buf(void *buf) {};
public:


    /**
     * 输出需要格式化的字符串
     *  但是 一次性格式化的字符个数 超过 缓冲区的长度
     *  会发生截断
     * @param buf 格式化所需的缓冲区
     * @param buf_len 格式化所需的缓冲区长度
     *                  但是不可以超过int最大值
     * @param cr 是否需要以'\n'为结束符号 即是否换行
     * @param format 格式化参数模板
     * @param ap 可变参数的列表
     * @param avail_len 格式化后可用的长度
     * @return 错误状态码：
     *             NO_MEMORY :内存不足,缓冲区溢出
     *             ERR : 格式化错误
     *             OK ： 成功
     */
    static OSReturn do_vsnprintf(char *buf,
                                 size_t buf_len,
                                 bool cr,
                                 const char *format,
                                 va_list ap,
                                 int32_t *avail_len = nullptr);

    /**
     * 默认的构造函数 写入字符个数应该是0
     */
    inline explicit CharOStream() : OStream() {};

    /**
     * 析构函数
     */
    ~CharOStream() override = default;

    /**
     * 输出需要格式化的字符串
     *  但是 一次性格式化的字符个数 超过 O_LENGTH定义的缓冲区大小
     *  会发生截断
     * @param format
     * @param ...
     * @return >0 写入的长度
     *          <0 表示出现了错误，状态码看本类中的定义
     */
    OSReturn print(const char *format, ...);

    /**
     * 输出需要格式化的字符串 并以换行符 ‘\n’ 结束
     *  但是 一次性格式化的字符个数 超过 O_LENGTH定义的缓冲区大小
     *  会发生截断
     * @param format 格式化模板
     * @param ... 格式化所需的参数
     * @return >0 写入的长度
     *          <0 表示出现了错误，状态码看本类中的定义
     */
    OSReturn print_cr(const char *format, ...);

    /**
     * 输出换行符
     */
    inline void cr() {
        this->print_raw("\n", 1);
    };

    /**
     * 将字符串原封不动的写入到缓冲区中
     * @param str 字符串
     * @param len 字符串长度
     */
    inline void print_raw(const char *str, size_t len) {
        this->write_bytes((void *) str, len);
    };

    inline void print_raw(const char *str) {
        this->print_raw(str, ::strlen(str));
    };


    /**
     * 将字符串原封不动的写入到缓冲区中
     * 并输出换行符
     * @param str 字符串
     */
    inline void print_raw_cr(const char *str) {
        this->print_raw(str);
        this->cr();
    };
    inline void print_raw_cr(const char *str,size_t str_len) {
        this->print_raw(str,str_len);
        this->cr();
    };
    /**
     * 打印数据
     * @param data 数据开始的首地址
     * @param len 数据的长度
     * @param with_ascii 是否输出对应的ASCII
     */
    void print_data(void *data, size_t len, bool with_ascii);

    /**
     * 输出时间戳的字符串
     * @param prefix 输出前缀
     * @param suffix 输出后缀
     */
    void stamp_string(const char *prefix,
                      const char *suffix);

    inline void stamp_string() {
        this->stamp_string(" ", ":");
    };
    /**
     * 打印 符合 人阅读的字节
     * @param bytes
     */
    OSReturn print_human_bytes(size_t bytes);
};

#endif //PLATFORM_CHAR_OSTREAM_HPP
