//
// Created by aurora on 2025/1/20.
//
#include "iostream"
#include "platform/utils/BitMap.hpp"
#include "platform/main/init.hpp"
#include "platform/log.hpp"
#include "platform/concurrent/OSThread.hpp"
#include "platform/concurrent/VM_Operation.hpp"
#include "unistd.h"
#include "platform/concurrent/PeriodicTask.hpp"

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
    Platform::pre_initialize();
    Platform::global_initialize();
    TestVM_Operation testVmOperation;
    TestVM_Operation::execute(&testVmOperation);


//    TestPeriodicTask task(100);
//    task.activate();
//    sleep(10);

    Platform::destroy();
    return 0;
}