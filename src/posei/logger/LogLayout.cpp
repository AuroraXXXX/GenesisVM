//
// Created by aurora on 2022/2/18.
//

#include "plat/logger/LogLayout.hpp"
#include <cstring>
#include "plat/stream/CharOStream.hpp"
/**
 * 日志布局器的某个参数对应的名称
 */
const char *LogLayout::_name[][2] =
        {
#define LOG_LAYOUT_PARAM(name, compress) {#name,#compress},
                LOG_LAYOUT_PARAM_LIST
#undef LOG_LAYOUT_PARAM
        };
const LogLayout LogLayout::Default = LogLayout(LogLayout::DefaultMask);
/**
 * 将字符串转换成对应的某个日志布局参数的枚举值
 * @param str 字符串
 * @return 日志布局参数对应的枚举值
 */
LogLayout::Type LogLayout::from_string(const char *str) {
    for (size_t i = 0; i < Count; i++) {
        /**
         * 按照 日志布局器参数的声明顺序
         * 获取对应的日志布局器的参数
         * 仅仅需要强制类型转换即可
         */
        auto d = static_cast<LogLayoutType>(i);
        /**
         * 忽略大小写 对输出的字符串 和 对应日志布局器的参数名称 进行比较
         * 由于存在缩写 所以比中任一即可
         */
        if (::strcasecmp(str, LogLayout::name(d)) == 0 ||
            ::strcasecmp(str, LogLayout::compress_name(d)) == 0) {
            return d;
        }
    }
    /**
     * 没找到
     */
    return Invalid;
}

bool LogLayout::parse(const char *params_args, CharOStream *err_stream) {
    /**
     * 没有指定日志布局器
     */
    if (params_args == nullptr || strlen(params_args) == 0) {
        this->_value = 0;
        return true;
    }
    /**
     * 指定为none也是认为是没有指定信息
     */
    if (strcasecmp(params_args, "none") == 0) {
        this->_value = 0;
        return true;
    }
    /**
     * 解析的结果
     */
    bool result = true;
    uint32_t temp_value = 0;
    /**
     * 拷贝字符串 因为const char* 指向的字符串极有可能是写在代码中的 我们是无法直接修改的
     * 所以需要拷贝一份
     */
    size_t len = strlen(params_args);
    char copy[len];
    strcpy(copy, params_args);
    //首先指向拷贝的字符串的首地址 下面开始进行解析
    char *token = copy;
    //这个指针用于指向分割符
    char *comma_pos;
    do {
        //首先查找分割符，
        comma_pos = strchr(token, ',');
        /**
         * 如果找到了 那么这个分割符指针 存储的地址就不是nullptr
         * 就需要把分割符设置为 字符串的结束符 '\0' 方便下面将字符串转化成对应的枚举值
         */
        if (comma_pos != nullptr) {
            *comma_pos = '\0';
        }
        LogLayoutType d = from_string(token);
        /**
         * 得到的枚举类型 是Invalid 说明没找到
         * 那么应该输出错误信息
         */
        if (d == Invalid) {
            if (err_stream != nullptr) {
                err_stream->print_cr("非法的日志布局器参数: '%s'", token);
            }
            result = false;
            break;
        }
        /**
         * 说明解析成功了 那么需要将这个布局参数记录在对应的比特位上
         */
        temp_value |= LogLayout::mask(d);
        /**
         * 下一次需要解析的token 就在分割符的下一个字节
         * 如果 没有找到分割符 此次操作就是白做 无影响
         */
        token = comma_pos + 1;
        /**
         * 结束条件就是分割符没有找到
         */
    } while (comma_pos != nullptr);
    /**
     * 如果 解析成功 那么就需要设置
     */
    if (result) {
        this->_value = temp_value;
    }
    return result;
}
