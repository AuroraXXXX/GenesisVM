//
// Created by aurora on 2023/11/25.
//

#include "plat/stream/FileCharOStream.hpp"
#include <cstdio>
#include "plat/utils/robust.hpp"

FileCharOStream FileCharOStream::_err_stream(stderr);
FileCharOStream FileCharOStream::_out_stream(stdout);

void FileCharOStream::write(const void *data, size_t data_len) {
   ::fwrite(data, data_len, 1, (FILE *) (this->_file));
}

void FileCharOStream::lock() {
    ::flockfile((FILE *) (this->_file));
}

void FileCharOStream::unlock() {
    ::funlockfile((FILE *) (this->_file));
}


FileCharOStream::FileCharOStream(
        const char *path,
        const char *mode,
        CacheMode cache) noexcept
        : CharOStream(),
          _file(nullptr),
          _need_close(true) {
    auto file = ::fopen(path, mode);
    this->_file = file;
    if (file == nullptr) {
        return;
    }
    switch (cache) {
        case CacheMode::NoCache:
            ::setvbuf(file, nullptr, _IONBF, 0);
            break;
        case CacheMode::LineCache:
            ::setvbuf(file, nullptr, _IOLBF, 0);
            break;
        case CacheMode::AllCache:
            ::setvbuf(file, nullptr, _IOFBF, 0);
            break;
    }
}

FileCharOStream::FileCharOStream(
        void *file,
        bool need_close) noexcept:
        _file(file),
        _need_close(need_close) {
    assert(this->_file != nullptr, "file must not null");
}


void FileCharOStream::close() {
    if (this->_need_close && this->_file != nullptr) {
        ::fclose((FILE *) (this->_file));
        this->_file = nullptr;
    }
}

void FileCharOStream::flush() {
    ::fflush((FILE *) this->_file);
}



void FileCharOStream::flush_default_stream() {
    const auto out_stream = &FileCharOStream::_out_stream;
    const auto err_stream = &FileCharOStream::_err_stream;
    assert(out_stream != nullptr, "check");
    assert(err_stream != nullptr, "check");
    out_stream->flush();
    err_stream->flush();
}








