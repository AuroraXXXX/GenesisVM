//
// Created by aurora on 2025/1/20.
//
#include "iostream"
#include "platform/utils/BitMap.hpp"
#include "platform/main/init.hpp"
#include "platform/log.hpp"
#include "platform/concurrent/OSThread.hpp"
#include "../src/daemon-thread/VMThread.hpp"
#include "daemon-thread/VM_Operation.hpp"
#include "unistd.h"

class TestVM_Operation:public VM_Operation{
public:
    void doit() override {
        std::cout<<"hello world"<<std::endl;
    }

    const char *name() override {
        return "test vm operation";
    }
};

int main() {
    platform_init();
    log_error(platform)("adv");
    VMThread::create();
    TestVM_Operation testVmOperation;
    TestVM_Operation::execute(&testVmOperation);
    sleep(2000);
    return 0;
}