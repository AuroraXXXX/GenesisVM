//
// Created by aurora on 2023/9/10.
//

#ifndef plat_MEM_ALLOCATION_HPP
#define plat_MEM_ALLOCATION_HPP

#include "plat/utils/NativeCallStack.hpp"
#include "stdtype.hpp"
#include <new>
#include "AllStatic.hpp"
/**
 * CHeapObject内存的标记
 * 申请的都是C++对象
 *
 * mmap 是对应管理的内存
 */
#define MEMORY_FLAGS_DO(f)                                 \
f(LangHeap,"用于支持语言层面的堆")                            \
f(Class,"用于支持语言层面的对象")                              \
f(Thread,"用于表示线程对象")                                  \
f(ThreadStack,"用于表示线程栈")                               \
f(GC,"用于支持GC")                                           \
f(Internal,"内部内存,无法内存追踪")                            \
f(Symbol,"用于系统符号")                                      \
f(Chunk,"用于快速内存")                                          \
f(Arena,"用于Arena")                                      \
f(Module,"模块")                       \
f(Logging,"用于支持日志系统")                                 \
f(Metaspace,"用于元空间的内存")                               \
f(Management,"用于支持内存管理")                              \
f(Arguments,"用于支持参数")                                   \
f(Synchronizer,"用于表示锁对象")                              \
f(None,"未定义")


enum class MEMFLAG : uint8_t {
#define MEMORY_FLAG_DECLARE_ENUM(type, human_readable) type,
    MEMORY_FLAGS_DO(MEMORY_FLAG_DECLARE_ENUM)
#undef MEMORY_FLAG_DECLARE_ENUM
    num_of_type
};


class NativeCallStack;

class CharOStream;

/**
 * 获取内存标记的字符串
 * @param F
 * @return
 */
const char *MEMFLAG_NAME(MEMFLAG F);

extern void *CHEAP_ALLOC(MEMFLAG F,
                         size_t bytes,
                         bool exit_oom = true);


/**
 * 申请内存 并要求对齐到指定字节
 * @param F
 * @param bytes
 * @param align
 * @param caller
 * @param exit_oom  true    表示申请失败退出虚拟机
 *                  false   表示申请失败返回null
 * @return
 */
extern void *CHEAP_ALLOC_ALIGN(
        MEMFLAG F,
        size_t bytes,
        size_t align,
        bool exit_oom = true);


extern void CHEAP_FREE(MEMFLAG F, void *p);

#define NEW_CHEAP_ARRAY(type, len, F) \
    (type*)CHEAP_ALLOC(F,sizeof(type)*(len))
#define FREE_CHEAP_ARRAY(old_ptr, F) \
    CHEAP_FREE(F,(void*)(old_ptr))

/**
 * 表示所申请的内存直接是在C的heap上申请的
 */
template<MEMFLAG F>
class CHeapObject {
public:
    inline void operator delete(void *ptr) {
        CHEAP_FREE(F, ptr);
    };

    inline void operator delete[](void *ptr) {
        CHEAP_FREE(F, ptr);
    };

    inline void *operator new[](size_t size) {
        return CHEAP_ALLOC(F, size);
    };

    inline void *operator new(size_t size) {
        return CHEAP_ALLOC(F, size);
    }

    [[nodiscard]] inline const char *flag_name() const {
        return MEMFLAG_NAME(F);
    };
};


/**
* 仅可以分配到栈上
*/
class StackObject {
public:
    void *operator new(size_t size) = delete;

    void operator delete(void *p) = delete;

    void *operator new[](size_t size) = delete;

    void operator delete[](void *p) = delete;
};


class Arena;

/**
 * -----------------------------
 * Arena 内存申请和分配
 * -----------------------------
 */
extern void *ARENA_ALLOC(
        Arena *arena,
        size_t bytes,
        bool exit_oom);

extern void ARENA_FREE(
        Arena *arena,
        void *ptr,
        size_t bytes);

/**
 * -----------------------------
 * 线程栈资源申请和分配
 * -----------------------------
 */
extern void *RESOURCE_ARENA_ALLOC(
        size_t bytes,
        bool exit_oom = true);

extern void RESOURCE_ARENA_FREE(void *ptr, size_t bytes);


#define NEW_RESOURCE_ARRAY(type, size)\
  (type*) RESOURCE_ARENA_ALLOC((size) * sizeof(type))

#define FREE_RESOURCE_ARRAY(ptr,bytes) \
    RESOURCE_ARENA_FREE(ptr,bytes)

#define NEW_ARENA_ARRAY(arena, type, size)\
  (type*) ARENA_ALLOC((arena),(size) * sizeof(type))

/**
 * 虚拟机内部申请的快速内存
 *
 * 默认情况下，资源区域中分配的对象的基类。
 * delete 是无效的
 * 在回到标记点时会之前释放之前申请到的内存
 */
class ResourceObject {
public:

    /**
     * 资源区域申请的内存不应该被显式的删除
     * @param p
     */
    void operator delete(void *p) {};

    /**
     * 资源区域申请的内存不应该显式的删除
     * @param p
     */
    void operator delete[](void *p) {};

    inline void *operator new(size_t size) {
        return RESOURCE_ARENA_ALLOC(size);
    };

    inline void *operator new(size_t size, int len) {
        return RESOURCE_ARENA_ALLOC(size + len);
    };

    inline void *operator new[](size_t size) {
        return RESOURCE_ARENA_ALLOC(size);
    };
};
#endif //plat_MEM_ALLOCATION_HPP
