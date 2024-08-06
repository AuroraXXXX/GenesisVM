//
// Created by aurora on 2023/11/25.
//

#ifndef PLATFORM_FILE_CHAR_OSTREAM_HPP
#define PLATFORM_FILE_CHAR_OSTREAM_HPP

#include "global/flag.hpp"
#include "CharOStream.hpp"

class FileCharOStream : public CharOStream {
public:
    enum class CacheMode : uint8_t {
        NoCache,//不缓冲
        LineCache,//行缓冲
        AllCache //全缓冲
    };
private:
    void *_file;
    bool _need_close;

protected:
    void write(const void *data, size_t data_len) override;

public:
    /**
     * 构造函数
     * @param path 文件的路径
     * @param mode 文件打开的方式
     * @param cache 缓冲的方式
     */
    explicit FileCharOStream(const char *path,
                             const char *mode = "w",
                             CacheMode cache = CacheMode::AllCache) noexcept;

    explicit FileCharOStream(void *file, bool need_close = false) noexcept;

    inline ~FileCharOStream() override {
        this->close();
    };

    void close();

    /**
     * 锁定文件
     */
    void lock() override;

    /**
     * 解除文件的锁定
     */
    void unlock() override;

    /**
     * 判断文件是否打开
     * @return
     */
    [[nodiscard]] inline bool is_open() const { return this->_file != nullptr; };

    void flush() override;

private:
    /**
     * 错误输出流
     */
    static FileCharOStream _err_stream;
    /**
     * 标准输出流
     */
    static FileCharOStream _out_stream;
public:
    /**
     * 返回的是默认输出流对象
     * @return
     */
    static inline FileCharOStream *default_stream() {
        return global::OutputToStderr ?
               &FileCharOStream::_err_stream :
              & FileCharOStream::_out_stream;
    };

    static inline CharOStream *error_stream() {
        return &FileCharOStream::_err_stream;
    }

    static void flush_default_stream();
};


#endif //PLATFORM_FILE_CHAR_OSTREAM_HPP
