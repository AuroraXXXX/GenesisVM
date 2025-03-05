//
// Created by aurora on 2025/1/20.
//
#include "iostream"
#include "platform/utils/BitMap.hpp"
#include "platform/main/init.hpp"
#include "platform/log.hpp"
#include "platform/concurrent/OSThread.hpp"
#include "unistd.h"


class TestThread : public UserThread{
public:
    void run() override {
        std::cout <<"hello world"<<std::endl;
    }
};

int main() {
    platform_init();
    log_error(platform)("adv");
    auto thread =new TestThread();
    os::create_thread(thread);

    return 0;
}