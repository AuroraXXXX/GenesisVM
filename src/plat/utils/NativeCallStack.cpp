//
// Created by aurora on 2023/9/27.
//

#include "plat/utils/NativeCallStack.hpp"
#include <cstring>


NativeCallStack NativeCallStack::_empty_stack;

NativeCallStack::NativeCallStack() noexcept {
    ::memset(this->_stack, 0, sizeof(this->_stack));
}

NativeCallStack::NativeCallStack(void **pc, uint16_t count) {
    auto copyNum = MIN2<int>(count, NativeCallStack::MAX_DEPTH);
    int32_t i;
    for (i = 0; i < copyNum; ++i) {
        this->_stack[i] = pc[i];
    }
    for (; i < NativeCallStack::MAX_DEPTH; ++i) {
        this->_stack[i] = nullptr;
    }
}
void NativeCallStack::copy_from(const NativeCallStack &call_stack) {
    for (int32_t i = 0; i < MAX_DEPTH; ++i) {
        this->_stack[i] = call_stack._stack[i];
    }
}
int NativeCallStack::frames() {
    int i;
    for (i = 0; i < NativeCallStack::MAX_DEPTH; ++i) {
        if (this->_stack[i] == nullptr) {
            break;
        }
    }
    return i;
}


void *NativeCallStack::top() const {
    int i = NativeCallStack::MAX_DEPTH;
    for (; i > 0; --i) {
        auto value = this->_stack[i];
        if (value != nullptr) {
            return value;
        }
    }
    return nullptr;
}

void NativeCallStack::record_current_caller() {
    for (auto &i: this->_stack) {
        if (i == nullptr) {
            i = current_thread_pc();
        }
    }
}

NativeCallStack::NativeCallStack(uint16_t depth) {
    depth = MIN2<uint16_t>(depth, MAX_DEPTH);
#define stack_case(depth) case depth:  _stack[depth] = return_thread_pc<depth>();
    switch (depth) {
        stack_case(4);
        stack_case(3);
        stack_case(2);
        stack_case(1);
        stack_case(0);
        break;
        default:
            break;
    }
#undef stack_case
}







