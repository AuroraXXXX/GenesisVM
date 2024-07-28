#include <iostream>
#include "plat/os/time.hpp"
#include "plat/logger/log.hpp"
#include "plat/stream/FileCharOStream.hpp"
#include "plat/PlatInitialize.hpp"
#include "kernel/thread/LangThread.hpp"
using namespace std;
enum {
    PREFIX_LOG_TAG(os)
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
    return 0;
}
