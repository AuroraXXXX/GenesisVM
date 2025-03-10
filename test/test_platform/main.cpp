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
#include "daemon-thread/PeriodicTask.hpp"
#include "../../src/daemon-thread/PeriodicThread.hpp"
class TestVM_Operation:public VM_Operation{
public:
    void doit() override {
        std::cout<<"hello world"<<std::endl;
    }

    const char *name() override {
        return "test vm operation";
    }
};

class TestPeriodicTask :public PeriodicTask{
protected:
    void task() override {
        std::cout<<"test..."<<std::endl;
    }

public:
    explicit TestPeriodicTask(uint32_t intervalCount) : PeriodicTask(intervalCount) {}

};
int main() {
    platform_init();
    log_error(platform)("adv");
//    VMThread::create();
//    TestVM_Operation testVmOperation;
//    TestVM_Operation::execute(&testVmOperation);
//    sleep(2000);

    PeriodicThread::create();

    TestPeriodicTask task(100);
    task.activate();

    sleep(5);
    PeriodicThread::start();
    sleep(20);
    return 0;
}