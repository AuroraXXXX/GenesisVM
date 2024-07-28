//
// Created by aurora on 2024/6/24.
//
#include <dlfcn.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/mman.h>
#include "plat/os/file.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <climits>

namespace os {
    void *so_load(const char *path_name, char *err_buf, int32_t err_buf_len) {
        const auto result = ::dlopen(path_name, RTLD_LAZY);
        if (result != nullptr) {
            return result;
        }
        const char *error_report = ::dlerror();
        if (error_report == nullptr) {
            error_report = "dlerror returned no error description";
        }
        if (err_buf && err_buf_len > 0) {
            ::strncpy(err_buf, error_report, err_buf_len - 1);
            err_buf[err_buf_len - 1] = '\0';
        }
        return nullptr;
    }


    void so_unload(void *so) {
        ::dlclose(so);
    }

    void *so_lookup(void *so, const char *name) {
        return ::dlsym(so, name);
    }

    FileType stat(const char *path,
                  size_t *file_size,
                  size_t *block_size) {
        struct stat64 file_struct{};
        if (::stat64(path, &file_struct) != 0) {
            return FileType::not_exist;
        }
        FileType res;
        const auto mode = file_struct.st_mode;
        if (S_ISDIR(mode)) {
            res = FileType::dir;
        } else if (S_ISREG(mode)) {
            res = FileType::regular;
        } else if (S_ISFIFO(mode)) {
            res = FileType::fifo;
        } else {
            res = FileType::unknown;
        }
        if (file_size != nullptr) {
            *file_size = file_struct.st_size;
        }
        if (block_size != nullptr) {
            *block_size = file_struct.st_blksize;
        }
        return res;
    }

    int32_t create_temp_mem_fd(const char *debug_name, size_t bytes) {
        auto fd = (int32_t) ::memfd_create(debug_name, MFD_CLOEXEC);
        if (fd == -1) {
            return fd;
        }
        if (::ftruncate64(fd, bytes) != 0) {
            ::close(fd);
            return -1;
        }
        return fd;
    }


#define HAS_FLAG(flag, type) ((flag) & (type))

    int32_t open(const char *path, int32_t fd_flags,size_t trunc_bytes) {
        //检查文件的长度
        if (::strlen(path) > PATH_MAX - 1) {
            errno = ENAMETOOLONG;
            return -1;
        }
        int32_t o_flag = O_RDONLY;
        if (HAS_FLAG(fd_flags, fd_read)) {
            if (HAS_FLAG(fd_flags, fd_write)) {
                o_flag = O_RDWR;
            } else {
                o_flag = O_RDONLY;
            }
        } else if (HAS_FLAG(fd_flags, fd_write)) {
            o_flag = O_WRONLY;
        }


        if (HAS_FLAG(fd_flags, fd_exec)) {
            o_flag |= O_EXCL;
        } else if (HAS_FLAG(fd_flags, fd_auto_close)) {
            o_flag |= O_CLOEXEC;
        } else if (HAS_FLAG(fd_flags, fd_temp)) {
            o_flag |= O_TMPFILE;
        } else if (HAS_FLAG(fd_flags, fd_create)) {
            o_flag |= O_CREAT;
        }
        int32_t mode = S_IRWXU;
        auto fd = ::open64(path, o_flag, mode);
        if(fd == -1){
            return fd;
        }
        if(trunc_bytes == 0){
            //不要求强制保留的内存大小
            return fd;
        }
        int res =  ::ftruncate64(fd,trunc_bytes);
        if(res == 0){
            return fd;
        } else{
            //失败了 直接返回
            ::close(fd);
            return -1;
        }
    }

#undef HAS_FLAG

    void close(int32_t fd) {
        ::close(fd);
    }
}