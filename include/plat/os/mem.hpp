//
// Created by aurora on 2024/6/24.
//

#ifndef PLATFORM_OS_MEM_HPP
#define PLATFORM_OS_MEM_HPP

#include "stdtype.hpp"
#include "plat/mem/allocation.hpp"
class CharOStream;
namespace os {
    /**
     * 获取页框的大小
     * @return
     */
    int32_t page_size();

    /**
     * 获取全部的页框的个数
     * @return
     */
    long total_pages();

    /**
     * 获取可用的页框的个数
     * @return
     */
    long avail_pages();

    /**
     * 保留虚拟地址空间
     * @param F 类型标记
     * @param bytes 申请的字节数
     * @param fd 文件句柄描述符
     * @return 内存地址 nullptr表示申请失败
     */
    void *reserve_memory(MEMFLAG F, size_t bytes, int32_t fd = -1);

    /**
     * 保留虚拟地址空间
     * @param F 类型标记
     * @param bytes 申请的字节数
     * @param align 对齐的粒度
     * @param fd 文件句柄描述符
     * @return 内存地址 nullptr表示申请失败
     */
    void *reserve_memory_aligned(MEMFLAG F,
                                  size_t bytes,
                                  size_t align,
                                  int32_t fd = -1);

    /**
     * 尝试保留虚拟地址空间
     * @param F 类型标记
     * @param addr 想要保留的地址
     * @param bytes 申请的字节数
     * @param fd 文件句柄描述符
     * @return 内存地址 nullptr表示申请失败
     */
    void *reserve_memory_at(MEMFLAG F,
                             void *addr,
                             size_t bytes,
                             int32_t fd = -1);

    /**
     * 强制保留虚拟地址空间，之前保留的地址会被释放
     * @param F 类型标记
     * @param addr 想要保留的地址
     * @param bytes 申请的字节数
     * @param fd 文件句柄描述符
     * @return 内存地址 nullptr表示申请失败
     */
    void *reserve_memory_force_at(MEMFLAG F,
                                   void *addr,
                                   size_t bytes,
                                   int32_t fd = -1);

    enum class CommitType {
        none = 0,
        //可读 可写 可执行
        rwx = 0x01,
        //可读 可写
        rw = 0x02,
        //可读
        r = 0X04,
    };

    /**
     * 提交内存
     * @param F 类型的标记
     * @param addr 想要保留的地址
     * @param bytes 申请的字节数
     * @param type 提交的类型
     * @return 操作是否成功
     */
    bool commit_memory(MEMFLAG F,
                       void *addr,
                       size_t bytes,
                       CommitType type);

    /**
     * 撤销内存的提交并且会将内部的数据清除
     * @param F 申请的内存标记
     * @param addr 虚拟进程地址
     * @param bytes 虚拟进程地址空间长度
     * @return 操作是否成功
     */
    bool uncommit_memory(MEMFLAG F,
                         void *addr,
                         size_t bytes);

    /**
     * 释放保留虚拟进程地址空间
     * @param F 申请的内存标记
     * @param addr 释放的进程空间
     * @param bytes 虚拟进程空间大小
     * @return 操作是否成功
     */
    bool release_memory(MEMFLAG F, void *addr, size_t bytes);

    /**
     * @param start 虚拟地址起始位置
     * @param bytes 虚拟地址空间大小
     */
    void pretouch_memory(void *start, size_t bytes);

    /**
     * 内存的dump
     * @param stream 目的输出流
     * @param addr 虚拟进程地址
     * @param bytes 虚拟进程地址空间长度
     * @param unit_bytes 每组显示的字节数
     * @param per_line_bytes 每行显示的字节数
     */
    void dump_memory(CharOStream *stream,
                     void *addr,
                     size_t bytes,
                     int32_t unit_bytes = 2,
                     int32_t per_line_bytes = 16);
}
#endif //PLATFORM_OS_MEM_HPP
