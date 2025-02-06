//
// Created by aurora on 2025/1/20.
//
#include "iostream"
#include "platform/utils/BitMap.hpp"
#include "platform/main/init.hpp"
#include "platform/stream/FileCharOStream.hpp"
int main() {
    platform_init(0);
    auto map = new CHeapBitMap(MEMFLAG::Internal);
    map->resize(256, true);
    map->set_range(0,128);
    auto stream = FileCharOStream::default_stream();
    map->print_stat(stream);
    std::cout << "hello" << map->count_range(0,256) <<std::endl;
    return 0;
}