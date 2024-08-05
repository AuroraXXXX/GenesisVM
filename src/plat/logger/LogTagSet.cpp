//
// Created by aurora on 2022/10/17.
//

#include <cstdio>
#include "plat/utils/robust.hpp"
#include "plat/logger/LogTagSet.hpp"
#include "plat/logger/log.hpp"
#include "plat/logger/constants.hpp"
const LogTagSet LogTagSet::Default;
static const char * TAG_NAME_LIST[] ={
        "",
#define LOG_TAG_DEF(name) #name,
        LOG_TAG_LIST(LOG_TAG_DEF)
#undef LOG_TAG_DEF
};

LogTagSet::LogTagSet() noexcept:
        _tags{
                LOG_TAGS(no_tag)
        } {
}

LogTagSet::LogTagSet(LogTag tag0,
                     LogTag tag1,
                     LogTag tag2,
                     LogTag tag3,
                     LogTag tag4) noexcept:
        _tags{tag0,
              tag1,
              tag2,
              tag3,
              tag4} {

}

int LogTagSet::write_tags(char *buf, size_t buf_len, const char *split) {
    //记录向缓冲区写入的实际字符
    int total = 0;
    //对缓冲区进行清零
    buf[0] = '\0';
    bool is_first = true;
    for (LogTag tag: this->_tags) {
        if (tag == PREFIX_LOG_TAG(no_tag)) {
            //当遇到一个没有标记标签的时候 那么就应该停止输出
            break;
        }
        auto split_str = is_first ? "" : split;
        if (is_first) {
            is_first = false;
        }
        //下面开始输出标签
        auto writen = ::snprintf(
                //下一次可写的位置应该在上一次之后 所以应该是缓冲区首地址 + 现在已经写入的字节数
                buf + total,
                //缓冲区剩余的可用空间 当然是 缓冲区原本的长度 - 已经写入的字节数
                buf_len - total,
                "%s%s",
                //输出 分割符，第一个标签之前应该不输出分割符
                split_str,
                //输出标签名称
                TAG_NAME_LIST[(uint16_t)tag]);
        // 返回值 < 0 表明出现错误了
        assert(writen != -1, "日志的标签编码出现错误");
        if (writen < 0 || writen >= buf_len) {
            return -1;
        }
        total += writen;
    }
    return total;
}
















