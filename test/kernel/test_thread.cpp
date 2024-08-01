//
// Created by aurora on 2024/7/28.
//
#include <iostream>
#include "plat/os/time.hpp"
#include "plat/logger/log.hpp"
#include "plat/PlatInitialize.hpp"
#include "kernel/thread/LangThread.hpp"
#include "kernel/thread/PeriodicTask.hpp"
#include "kernel/thread/VM_Operation.hpp"
#include "kernel/KernelInitialize.hpp"
#include "unistd.h"
using namespace std;
enum {
    PREFIX_LOG_TAG(os),
    PREFIX_LOG_TAG(vmthread),
    PREFIX_LOG_TAG(safepoint)
};
class PeriodicTaskTest :public PeriodicTask{
protected:
    void task() override {
            cout<<"periodic ..."<<endl;
    }

public:
    inline explicit PeriodicTaskTest(uint32_t interval) : PeriodicTask(interval) {}

};
class VMOperationTest:public VM_Operation{
public:
    void doit() override {
        cout<<"vm operation test ..."<<endl;
    }

    const char *name() override {
        return "VMOperationTest";
    }
};
int main() {
    const auto stamp = os::current_stamp();

    const  char *tags_name[] = {
            "os",
            "vmthread",
            "safepoint"
    };
    OSThread *lang_thread = new LangThread();

    PlatInitialize::initialize(stamp,
                               tags_name,
                               sizeof(tags_name),
                               lang_thread);
    log_trace(os)("aa");
    KernelInitialize::initialize();
    PeriodicTaskTest test(100);
    test.activate();
    test.inactivate();
    VMOperationTest vmOperationTest;
    //为了等待内核线程创建完毕
    sleep(1);
    VMOperationTest::execute(&vmOperationTest);
    while (true) {}
    return 0;
}
