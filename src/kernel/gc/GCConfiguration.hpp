//
// Created by aurora on 2024/8/16.
//

#ifndef GENESISVM_GCCONFIGURATION_HPP
#define GENESISVM_GCCONFIGURATION_HPP

#include "constants.hpp"
#include "plat/mem/AllStatic.hpp"
#include "kernel/utils/GrowableArray.hpp"

namespace gc {
    /**
     * 将某个元素推入到栈中
     * @param ptr 存储指向heap指针的地址（二重地址）
     */
    using PushGCStack = void (*)(void **ptr);
    /**
     * 用于其他模块将堆上根推入到栈中
     * @param obj_func 将某个非数组元素的对象推入到栈中
     * @param array_func 将某个数组元素的对象推入到栈中
     */
    using MarkGCRootFunc = void (*)(PushGCStack obj_func, PushGCStack array_func);

    class GCConfiguration : public AllStatic {
    private:
        static MarkGCRootFunc mark_GC_func[GC_CALLBACK_ARRAY_LEN];
    public:
        static void register_mark_gcroot_func(MarkGCRootFunc func);
    };

}
#endif //GENESISVM_GCCONFIGURATION_HPP
