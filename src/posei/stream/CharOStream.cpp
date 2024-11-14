//
// Created by xu on 2023/8/26.
//

#include "plat/stream/CharOStream.hpp"
#include <cstdio>
#include "plat/macro.hpp"
#include "plat/utils/robust.hpp"
#include "plat/os/time.hpp"
#include "plat/utils/align.hpp"

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

static const char *get_unit_bytes(size_t bytes, CharOStream::HumanType scale, size_t *unit_bytes) {
    if (scale == CharOStream::HumanType::exact) {
        if (bytes >= T && is_aligned(bytes, T)) {
            *unit_bytes = T;
            return "TB";
        } else if (bytes >= G && is_aligned(bytes, G)) {
            *unit_bytes = G;
            return "GB";
        } else if (bytes >= M && is_aligned(bytes, M)) {
            *unit_bytes = M;
            return "MB";
        } else if (bytes >= K && is_aligned(bytes, K)) {
            *unit_bytes = K;
            return "KB";
        } else {
            *unit_bytes = 1;
            return "B";
        }
    } else {
        if (scale == CharOStream::HumanType::fuzzy) {
            if (bytes >= T) {
                scale = CharOStream::HumanType::fuzzy_t;
            } else if (bytes >= G) {
                scale = CharOStream::HumanType::fuzzy_g;
            } else if (bytes >= M) {
                scale = CharOStream::HumanType::fuzzy_m;
            } else if (bytes >= K) {
                scale = CharOStream::HumanType::fuzzy_k;
            } else {
                scale = CharOStream::HumanType::fuzzy_b;
            }
        }
        switch (scale) {
            default:
                should_not_reach_here();
            case CharOStream::HumanType::fuzzy_b:
                *unit_bytes = 1;
                return "B";
            case CharOStream::HumanType::fuzzy_k:
                *unit_bytes = K;
                return "KB";
            case CharOStream::HumanType::fuzzy_m:
                *unit_bytes = M;
                return "MB";
            case CharOStream::HumanType::fuzzy_g:
                *unit_bytes = G;
                return "GB";
            case CharOStream::HumanType::fuzzy_t:
                *unit_bytes = T;
                return "TB";
        }
    }
}

OSReturn CharOStream::print_human_bytes(size_t bytes, HumanType scale) {
    size_t unit_bytes;
    const auto unit_str = get_unit_bytes(bytes, scale, &unit_bytes);
    if (scale == HumanType::exact) {
        const auto show = bytes / unit_bytes;
        return this->print(SIZE_FORMAT "%s", show, unit_str);
    } else {
        const auto show = (double) bytes / (double) unit_bytes;
        return this->print("%.2fs", show, unit_str);
    }
}

OSReturn CharOStream::print_human_percent(int32_t part, int32_t total) {
    if (total == 0) {
        return this->print("  ?%%");
    } else if (part == 0) {
        return this->print("  0%%");
    } else if (part == total) {
        return this->print("100%%");
    } else {
        // Note: clearly print very-small-but-not-0% and very-large-but-not-100% percentages.
        float p = ((float) part / (float )total) * 100.0f;
        if (p < 1.0f) {
           return  this->print(" <1%%");
        } else if (p > 99.0f) {
            return this->print(">99%%");
        } else {
            return this->print("%3.0f%%", p);
        }
    }
}


