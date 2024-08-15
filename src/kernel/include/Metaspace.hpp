//
// Created by aurora on 2022/12/23.
//

#ifndef VMACHINE_METASPACE_HPP
#define VMACHINE_METASPACE_HPP
#include "plat/mem/AllStatic.hpp"
#include "stdtype.hpp"
class CharOStream;
namespace metaspace {
    class MetaspaceArena;

    class Metaspace : public AllStatic {
    public:

        /**
         * 用于设置元空间的参数
         */
        static void ergo_initialize();

        /**
         * 进行元空间的实际初始化
         */
        static void global_initialize();

        /**
         * 待元空间完成实际初始化后 进行一些后置的操作
         */
        static void post_initialize();

        /**
         * 尝试清除元空间中的内存
         * 应在类被卸载时候使用
         */
        static void purge();

        /**
         * 在GC情况下再次进行申请 应该获取锁的情况下
         * @param arena
         * @param bytes
         * @return
         */
        //static void *expand_allocate_with_gc(MetaspaceArena *arena, size_t bytes);

        /**
         * 打印元空间的基本信息
         * @param out
         */
        static void print_on(CharOStream *out);
    };
}

#endif //VMACHINE_METASPACE_HPP
