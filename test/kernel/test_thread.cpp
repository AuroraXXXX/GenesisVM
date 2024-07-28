//
// Created by aurora on 2024/7/28.
//
#include <iostream>
#include "plat/os/time.hpp"
#include "plat/logger/log.hpp"
#include "plat/PlatInitialize.hpp"
#include "kernel/thread/LangThread.hpp"
#include "kernel/thread/PeriodicTask.hpp"
#include "kernel/KernelInitialize.hpp"
using namespace std;
enum {
    PREFIX_LOG_TAG(os)
};
class PeriodicTaskTest :public PeriodicTask{
protected:
    void task() override {
            cout<<"periodic ..."<<endl;
    }

public:
    inline explicit PeriodicTaskTest(uint32_t interval) : PeriodicTask(interval) {}

};
int main() {
    const auto stamp = os::current_stamp();

    const  char *tags_name[] = {
            "os"
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
    while (true) {}
    return 0;
}
