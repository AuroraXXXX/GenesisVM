//
// Created by aurora on 2024/8/16.
//

#include "GCConfiguration.hpp"
gc::MarkGCRootFunc gc::GCConfiguration::mark_GC_func[gc::GC_CALLBACK_ARRAY_LEN];
void gc::GCConfiguration::register_mark_gcroot_func(gc::MarkGCRootFunc func) {
    for (MarkGCRootFunc& f: GCConfiguration::mark_GC_func) {
        if(f == nullptr){
            f = func;
            return;
        }
    }
    assert(false,"overflow mark gc root func array.");
}
