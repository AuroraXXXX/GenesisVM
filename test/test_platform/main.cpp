//
// Created by aurora on 2025/1/20.
//
#include "iostream"
#include "platform/utils/BitMap.hpp"
#include "platform/main/init.hpp"
int main() {
    platform_init(0);
    auto map = new CHeapBitMap(MEMFLAG::Internal);
    map->resize(60, true);

    std::cout << "hello" << std::endl;
    return 0;
}