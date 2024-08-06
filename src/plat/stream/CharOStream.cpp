//
// Created by xu on 2023/8/26.
//

#include "plat/stream/CharOStream.hpp"
#include <cstdio>
#include "plat/macro.hpp"
#include "plat/utils/robust.hpp"
#include "plat/os/time.hpp"

OSReturn CharOStream::do_vsnprintf(
        char *buf,
        size_t buf_len,
        bool cr,
        const char *format,
        va_list ap,
        int32_t *avail_len) {
    assert(buf_len < INT32_MAX, "缓冲区过大,超过最大值" INT_FORMAT, INT32_MAX);
    /**
     * 进行数据的格式化
     */
    auto need_len = ::vsnprintf(buf, buf_len, format, ap);
    if (need_len < 0) {
        //出现了编码错误
        if (avail_len) *avail_len = 0;
        return OSReturn::ERR;
    }
    OSReturn res;
    /**
     * 下面需要计算是否溢出缓冲区
     * 首先就是格式化的数据必须可以放入缓冲区并在缓冲区最后留下结束标记
     */
    if (need_len > buf_len - 1) {
        //说明溢出了
        res = OSReturn::NO_MEMORY;
        need_len = (int32_t) buf_len - 1;
    } else {
        res = OSReturn::OK;
    }
    //输出换行符
    if (cr) {
        buf[need_len++] = '\n';
    }
    if (avail_len) *avail_len = need_len;
    return res;
}


/**
 * example:
 * 0000000: 7f44 4f46 0102 0102 0000 0000 0000 0000  .DOF............
 * 0000010: 0000 0000 0000 0040 0000 0020 0000 0005  .......@... ....
 * 0000020: 0000 0000 0000 0040 0000 0000 0000 015d  .......@.......]
 *
 * @param data 需要打印的数据
 * @param len 数据的长度
 * @param with_ascii 是否输出对应的ascii
 */
void CharOStream::print_data(void *data, size_t len, bool with_ascii) {
    /**
    * 1行输出16字节的数据
    */
    size_t limit = (len + 16) / 16 * 16;
    for (size_t i = 0; i < limit; ++i) {
        //如果是行头 则输出编号
        if (i % 16 == 0) {
            this->print("%07x", i);
        }
        //两个字节之间 输出一个空格
        if (i % 2 == 0) {
            this->print(" ");
        }
        //输出16进制数据
        if (i < len) {
            this->print("%02x", ((unsigned char *) data)[i]);
        } else {
            this->print("  ");
        }
        //输出ASCII 这是在一行的最后
        if ((i + 1) % 16 == 0) {
            if (with_ascii) {
                //输出间隔符号
                this->print("  ");
                //打印ASCII码
                for (size_t j = 0; j < 16; ++j) {
                    //i -15 就是这行首元素的序号
                    size_t idx = i + j - 15;
                    if (idx < len) {
                        char c = ((char *) data)[idx];
                        //输出字符
                        this->print("%c", c >= 32 && c <= 126 ? c : '.');
                    }
                }
            }
            //最后换行符号
            this->print_cr("");
        }
    }
}

OSReturn CharOStream::print(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    auto res = this->do_vsnprintf_with_buf(false, format, ap);
    va_end(ap);
    return res;
}

OSReturn CharOStream::print_cr(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    auto res = this->do_vsnprintf_with_buf(true, format, ap);
    va_end(ap);
    return res;
}


OSReturn CharOStream::do_vsnprintf_with_buf(
        bool cr,
        const char *format,
        va_list ap) {

    int32_t avail_len;
    char *internal_buf = nullptr;
    {
        char buffer[OStreamDefaultBufSize];
        //进行格式化
        OSReturn res = CharOStream::do_vsnprintf(buffer, sizeof(buffer), cr, format, ap, &avail_len);

        //进行输出
        if (res == OSReturn::OK) {
            this->write_bytes(buffer, avail_len);
            return res;
        } else if (res != OSReturn::NO_MEMORY) {
            //其他错误直接返回
            return res;
        } else {
            //是缺少内存的错误 现在查询是否可以申请到其他内存
            internal_buf = (char *) this->alloc_internal_buf(avail_len + 1);
            if (internal_buf == nullptr)
                return res;
        }
    }

    //申请到了内存 需要再次进行格式化 然后进行输出
    OSReturn res = CharOStream::do_vsnprintf(internal_buf, avail_len + 1, cr, format, ap);
    assert(res == OSReturn::OK, "状态校验");
    this->write_bytes(internal_buf, avail_len);
    this->free_internal_buf(internal_buf);
    return OSReturn::OK;
}

void CharOStream::stamp_string(
        const char *prefix,
        const char *suffix) {
    this->print_raw(prefix);
    char buf[ISO8601_BUF_SIZE];
    os::iso8061(this->ticks(), buf, sizeof(buf), false);
    this->print_raw(buf);
    this->print_raw(suffix);
}
#define HAS_OTHER(value, unit) (((value) &((unit) - 1)) == 0)

static size_t print_human_exact_size(size_t bytes, const char **suffix) {
    if (bytes >= T) {
        auto value = bytes >> LogTB;
        *suffix = "TB";
        if (HAS_OTHER(bytes, T)) {
            return value;
        }
    }
    if (bytes >= G) {
        auto value = bytes >> LogGB;
        *suffix = "GB";
        if (HAS_OTHER(bytes, G)) {
            return value;
        }
    }
    if (bytes >= M) {
        auto value = bytes >> LogMB;
        *suffix = "MB";
        if (HAS_OTHER(bytes, M)) {
            return value;
        }
    }
    if (bytes >= K) {
        auto value = bytes >> LogKB;
        *suffix = "KB";
        if (HAS_OTHER(bytes, K)) {
            return value;
        }
    }
    *suffix = "B";
    return bytes;
}
#undef HAS_OTHER
OSReturn CharOStream::print_human_bytes(size_t bytes) {
    const char* unit;
    auto extact = ::print_human_exact_size(bytes,&unit);
    return this->print(SIZE_FORMAT "%s",extact,unit);
}


