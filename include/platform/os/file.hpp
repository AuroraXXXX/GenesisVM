//
// Created by aurora on 2024/6/24.
//

#ifndef PLATFORM_OS_FILE_HPP
#define PLATFORM_OS_FILE_HPP

#include "stdtype.hpp"
#include "plat/constants.hpp"

namespace os {
    void *so_load(const char *path_name, char *err_buf, int32_t err_buf_len);

    void so_unload(void *so);

    void *so_lookup(void *so, const char *name);

    enum class FileType {
        regular,//普通文件
        fifo,//管道文件
        dir,//文件夹
        not_exist,//不存在
        unknown
    };

    /**
     * 获取文件信息
     * @param path 文件的路径
     * @param file_size 文件的大小
     * @param block_size 文件组成的块的大小
     * @return 文件类型
     */
    FileType stat(const char *path,
                  size_t *file_size = nullptr,
                  size_t *block_size = nullptr);

    /**
     * 创建临时的内存文件
     * @param debug_name 用于调试的名称
     * @param bytes 所需的内存字节数
     * @return
     */
    int32_t create_temp_mem_fd(const char *debug_name, size_t bytes);

    enum {
        fd_read = 1 << 0,
        fd_write = 1 << 1,
        fd_exec = 1 << 2,//
        fd_temp = 1 << 3, //此时文件仅仅需要指定到文件夹
        fd_auto_close = 1 << 4, //退出时 进行关闭
        fd_create = 1 << 5 //文件不存在时进行创建
    };

    /**
     * 创建文件
     *
     * @param path 文件的路径
     * @param fd_flags 文件的描述符 如果时非法的参数会被忽略，但是至少保证是以只读方式打开
     * @param trunc_bytes 要求文件固定大小
     * @return
     */
    int32_t open(const char *path, int32_t fd_flags,size_t trunc_bytes);
    /**
     * 关闭输出
     * @param fd
     */
    void close(int32_t fd);

}
#endif //PLATFORM_OS_FILE_HPP
