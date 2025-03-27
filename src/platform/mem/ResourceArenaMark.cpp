//
// Created by aurora on 2025/3/27.
//
#include "platform/mem/ResourceArenaMark.hpp"
#include "platform/concurrent/OSThread.hpp"

ResourceArenaMark::ResourceArenaMark():
        ArenaMark(OSThread::current()->resource_arena()) {

}

ResourceArenaMark::~ResourceArenaMark() {
    this->rollback_to(OSThread::current()->resource_arena());
}
