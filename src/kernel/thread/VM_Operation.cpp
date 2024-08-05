//
// Created by aurora on 2024/2/27.
//

#include "kernel/thread/VM_Operation.hpp"
#include "kernel/thread/PlatThread.hpp"
#include "plat/logger/log.hpp"
#include "VMThread.hpp"
void VM_Operation::evaluate() {
    ResourceArenaMark mark;

    log_stream(debug,vmthread);
    if(log.is_enable()){
        log.print("begin ");
        this->print_on(&log);
    }
    this->doit();
    if (log.is_enable()){
        log.print("end ");
        this->print_on(&log);
    }
}

void VM_Operation::print_on(CharOStream *out) {
    out->print("VM_Operation (" PTR_FORMAT "): ", this);
    out->print("%s", this->name());

    out->print(", mode: %s", this->evaluate_at_safepoint() ? "safepoint" : "no safepoint");

    if (calling_thread()) {
        out->print(", requested by thread " PTR_FORMAT, this->calling_thread());
    }
}

void VM_Operation::execute(VM_Operation *operation) {
    assert(VMThread::current() != nullptr,"");
    VMThread::execute(operation);
}
