//
// Created by aurora on 2025/1/20.
//
#include "iostream"
#include "platform/utils/BitMap.hpp"
#include "platform/main/init.hpp"
#include "platform/stream/FileCharOStream.hpp"
#include "platform/log.hpp"
int main() {
    platform_init(0);
    log_error(LogTag::platform)("adv");

    return 0;
}