//
// Created by aurora on 2025/4/7.
//

#include "platform/main/init.hpp"
#include "../../src/metaspace/ContextHolder.hpp"
int main(){
    Platform::pre_initialize();
    Platform::global_initialize();

    using namespace metaspace;
    ContextHolder::global_initialize();

    Platform::destroy();
    return 0;
}
