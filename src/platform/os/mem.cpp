//
// Created by aurora on 2024/6/24.
//
#include "platform/os.hpp"
#include <unistd.h>
#include <sys/mman.h>
#include "platform/utils/robust.hpp"
#include <malloc.h>
#include "MemoryTracer.hpp"
#include "platform/stream/CharOStream.hpp"
#include "platform/utils/align.hpp"

/**
 * 获取页框的大小
 * @return
 */
int32_t os::page_size()
{
    static auto page_sizes = (int32_t)::sysconf(_SC_PAGE_SIZE);
    return page_sizes;
};

/**
 * 获取全部的页框的个数
 * @return
 */
long os::total_pages()
{
    return ::sysconf(_SC_PHYS_PAGES);
};

/**
 * 获取可用的页框的个数
 * @return
 */
long os::avail_pages()
{
    return ::sysconf(_SC_AVPHYS_PAGES);
}

/**
 * -------------------------
 * 内存映射
 * -------------------------
 */
/**
 * 匿名映射
 * @param request_addr 期望的地址 但是并不是强制的
 * @param bytes
 * @return
 */
static void *memory_mmap(void *request_addr, size_t bytes, int32_t fd, bool force)
{
    int flags;
    if (fd == -1)
    {
        /**
         * 标志中故意不指定MAP_FIXED 这样不会破坏原有的内存映射的完整性
         * 且不故意不指定地址，完全由系统决定分配
         *
         * MAP_NORESERVE 保留不使用交换空间，当没有物理页分配时候，那么会读写错误
         */
        flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
    }
    else
    {
        // 共享fd
        flags = MAP_SHARED;
    }
    if (request_addr != nullptr && force)
    {
        flags |= MAP_FIXED;
    }
    /**
     * 将保留/未提交的页面映射为PROT_NONE，以便在接触到未提交的页面时及早失败。
     */
    auto addr = ::mmap(request_addr,
                       bytes,
                       PROT_NONE,
                       flags,
                       fd,
                       0);
    return addr == MAP_FAILED ? nullptr : addr;
}

/**
 * 断开匿名映射
 * @param addr
 * @param size
 * @return
 */
static inline bool memory_unmap(void *addr, size_t size)
{
    return ::munmap(addr, size) == 0;
}

void *os::reserve_memory(MEMFLAG F, size_t bytes, int32_t fd)
{
    assert_is_aligned(bytes, os::page_size());
    assert(fd >= -1, "fd(%d) must be >= -1", fd);
    auto ptr = memory_mmap(nullptr, bytes, fd, false);
    if (ptr != nullptr)
    {
        MemoryTracer::record(F,
                             MemoryTracer::OperationType::reserve,
                             ptr,
                             bytes,
                             caller_address);
    }
    return ptr;
}

void *os::reserve_memory_aligned(MEMFLAG F, size_t bytes, size_t align, int32_t fd)
{
    /**
     * 计算总共需要映射的大小
     */
    assert_is_aligned(align, page_size());
    assert_is_aligned(bytes, align);
    const auto need_chip = align != os::page_size();
    const auto extra_size = need_chip ? bytes :  bytes + align;
    assert(extra_size >= bytes, "overflow,size太大,无法对齐");
    if (extra_size < bytes)
    {
        // 发生了数据溢出
        return nullptr;
    }
    // 获取可能大于原本要求的虚拟空间大小
    auto extra_base = reserve_memory(F, extra_size, fd);

    if (extra_base == nullptr)
    {
        // failed
        return nullptr;
    }
    char * aligned_base =(char*) extra_base;
    if(need_chip){
        /**
         * 对于多余的内存块应该进行切割下去 即使
         * [    |                     |     ]
         * ^    ^ aligned_base        ^ aligned_base + size
         * extra_base                       ^extra_base + extra_size
         */
        aligned_base = (char *)align_up<size_t>((size_t)extra_base, align);
        assert(is_aligned((size_t)aligned_base, align), "check");
        auto begin_offset = aligned_base - (char *)extra_base;
        auto end_offset = ((char *)extra_base + extra_size) - (aligned_base + bytes);
        if (begin_offset > 0)
        {
            release_memory(F, extra_base, begin_offset);
        }
        if (end_offset > 0)
        {
            release_memory(F, (char *)extra_base + begin_offset + bytes, end_offset);
        }
    }
    MemoryTracer::record(F,
                             MemoryTracer::OperationType::reserve,
                             aligned_base,
                             bytes,
                             caller_address);
    return aligned_base;
}

/**
 * 记录内存的申请 在某个地址
 */
void *reserve_memory_at(MEMFLAG F, void *addr, size_t bytes, int32_t fd, bool force)
{
    assert(addr != nullptr, "addr is not allow null");
    assert_is_aligned(bytes, os::page_size());
    assert_is_aligned((size_t)addr, os::page_size());
    auto real_addr = memory_mmap(addr, bytes, fd, true);
    if (force && real_addr != addr)
    {
        memory_unmap(real_addr, bytes);
        return nullptr;
    }
    // 记录内存的申请
    if(real_addr != nullptr){
            MemoryTracer::record(F,
                         MemoryTracer::OperationType::reserve,
                         real_addr,
                         bytes,
                         caller_address);
    }
    return real_addr;
}

bool os::release_memory(MEMFLAG F, void *addr, size_t bytes)
{
    assert_is_aligned((size_t)addr, page_size());
    assert_is_aligned(bytes, page_size());
    auto success = memory_unmap(addr, bytes);
    if (success)
    {
        MemoryTracer::record(F,
                             MemoryTracer::OperationType::release,
                             addr,
                             bytes,
                             caller_address);
    }
    return success;
}

bool os::commit_memory(MEMFLAG F, void *addr, size_t bytes, CommitType type)
{
    assert(addr != nullptr, "addr is not allow null");
    assert_is_aligned(bytes, page_size());
    assert_is_aligned((size_t)addr, page_size());
    int32_t prot_type;
    switch (type)
    {
    case CommitType::none:
        prot_type = PROT_NONE;
        break;
    case CommitType::r:
        prot_type = PROT_READ;
        break;
    case CommitType::rw:
        prot_type = PROT_READ | PROT_WRITE;
        break;
    case CommitType::rwx:
        prot_type = PROT_READ | PROT_WRITE | PROT_EXEC;
        break;
    }

    auto success = ::mprotect(addr, bytes, prot_type) == 0;
    if (success)
    {
        MemoryTracer::record(F,
                             MemoryTracer::OperationType::commit,
                             addr,
                             bytes,
                             caller_address);
    }
    return success;
}

bool os::uncommit_memory(MEMFLAG F, void *addr, size_t bytes)
{
    assert(addr != nullptr, "addr is not allow null");
    assert_is_aligned(bytes, page_size());
    assert_is_aligned((size_t)addr, page_size());
    auto success = ::mprotect(addr, bytes, PROT_NONE) == 0;
    if (success)
    {
        success = ::madvise(addr, bytes, MADV_DONTNEED) == 0;
    }
    if (success)
    {
        MemoryTracer::record(F,
                             MemoryTracer::OperationType::uncommit,
                             addr,
                             bytes,
                             caller_address);
    }
    return success;
}

void os::pretouch_memory(void *start, size_t bytes)
{
    auto end = (char *)start + bytes;
    auto page_bytes = page_size();
    for (auto p = (char *)start;
         p < end;
         p += page_bytes)
    {
        *p = 0;
    }
}

void os::dump_memory(CharOStream *stream,
                     void *addr,
                     size_t bytes,
                     int32_t unit_size,
                     int32_t bytes_per_line)
{
    auto start = (void *)align_down((size_t)addr, unit_size);
    auto end = (void *)align_up((size_t)addr + bytes, unit_size);
    assert(start <= end, "overflow");
    const auto cols_per_line = bytes_per_line / unit_size;
    int32_t cols = 0;
    auto p = start;
    // 打印 地址
    stream->print(PTR_FORMAT
                  ":\t",
                  p);
    while (p < end)
    {
        switch (unit_size)
        {
        case sizeof(uint8_t):
            stream->print("%02x", *(uint8_t *)p);
            break;
        case sizeof(uint16_t):
            stream->print("%04x", *(uint16_t *)p);
            break;
        case sizeof(uint32_t):
            stream->print("%08x", *(uint32_t *)p);
            break;
        case sizeof(uint64_t):
            stream->print("%16lx", *(uint64_t *)p);
            break;
        default:
            should_not_reach_here();
        }
        p = (char *)p + unit_size;
        ++cols;
        if (cols >= cols_per_line && p < end)
        {
            cols = 0;
            stream->cr();
            stream->print(PTR_FORMAT
                          ":\t",
                          p);
        }
        else
        {
            stream->print(" ");
        }
    }
    stream->cr();
}
