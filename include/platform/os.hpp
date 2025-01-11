#ifndef POSEI_OS_HPP
#define POSEI_OS_HPP
#include "typedef.hpp"
#include "platform/allocation.hpp"
#include "platform/main/global.hpp"

class OSThread;

class os : public AllStatic {
    friend void platform_init(ticks_t vm_start_time,
                           OSThread *os_thread);

private:
    /**
     * VM启动的时间戳
     */
    static ticks_t _vm_start_stamp;

    /**
     * 时间模块的初始化
     * vm_start_stamp VM启动得到时间戳
     */
    static void time_initialize(ticks_t vm_start_stamp);

public:
    /**
     * 获取进程的 运行时间
     * @param process_real_time 总的运行时间
     * @param process_user_time 用户态运行时间
     * @param process_system_time 内核态运行时间
     * @return
     */
    static bool proc_cpu_time(double &process_real_time,
                              double &process_user_time,
                              double &process_system_time);

    /**
     * 返回距离1970计算机元年的纳秒数
     * @return
     */
    static ticks_t current_stamp();

    /**
     * 返回距离启动的纳秒数
     * @return
     */
    inline static ticks_t elapsed_stamp() {
        return os::current_stamp() - os::_vm_start_stamp;
    };

    /**
     * 格式化字符串
     * 2023-11-20T00:23:00.000+08:00
     *
     * UTC
     * 2023-11-20T00:23:00.000Z
     * @param current_stamp 时间戳
     * @param buf 缓冲区地址
     * @param buf_len 缓冲区长度 必须大于ISO8601_BUF_SIZE
     * @param utc true  显示0时区时间
     *            false 显示当前时区时间
     * @return -1 表示出现错误
     */
    static int32_t iso8061(ticks_t current_stamp,
                           char *buf,
                           size_t buf_len,
                           bool utc,
                           uint8_t decimals = 3);

    /**
     *
     * @param current_stamp
     * @param format 见 strftime 函数
     * @param buf
     * @param buf_len
     * @param utc
     * @return
     */
    static OSReturn time_stamp_str(ticks_t current_stamp,
                                   const char *format,
                                   char *buf,
                                   size_t buf_len,
                                   bool utc);

    /**
     * ---------------------
     * thread 相关
     * ---------------------
     */

private:
    /**
     * 用于初始化内部的VM优先级到OS的优先级的映射
     * 不论是否修改这个映射关系，都需要调用这个函数
     * 内部会自行进行判断
     */
    static void native_prio_initialize();

    /**
     * 当 *uaddr == tag时 挂起线程
     * @param uaddr
     * @param tag 标志 当此值等于uaddr,则进入睡眠
     * @param nsec 超时等待多少ns 0 表示无限期等待
     */
    void suspend(const int *uaddr, int tag, uint64_t nsec = 0);

    /**
     * 唤醒等待在uaddr上的num个线程线程
     * @param uaddr int类型变量
     * @param num 线程的数量(如果大于实际等待的线程数量，则唤醒全部线程)
     * @return 返回实际被唤醒的线程数
     */
    int wakeup(int *uaddr, int num);

public:
    /**
     * 获取可用的CPU数量
     * @return
     */
    static uint32_t avail_cpu_num();

    /**
     * 获取全部的CPU数量
     * @return
     */
    static uint32_t total_cpu_num();

    /**
     * 获取当前线程所在的CPU序号
     * @return
     */
    static uint32_t current_cpu_id();

    static inline bool is_MP() { return avail_cpu_num() != 1; }

    /**
     * 获取线程在系统中的ID 只有第一次会进行调用系统
     * @return
     */
    static int32_t current_thread_id();

    /**
     * 获取当前系统进程的ID 只有第一次会进行调用系统
     * @return
     */
    static int32_t current_process_id();

    /**
     * 获取OS本身的线程优先级
     * @param thread_id OS的线程ID
     * @param native_prio OS线程实际设置的线程优先级
     * @return
     */
    static OSReturn get_native_prio(int32_t thread_id,
                                    int16_t *native_prio);

    /**
     * 设置OS的线程优先级
     * @param thread_id OS的线程ID
     * @param lang_prio ThreadPriority 规定的线程优先级
     * @return 操作状态码
     */
    static OSReturn set_native_prio(int32_t thread_id,
                                    ThreadPriority lang_prio);

    /**
     * 创建线程
     * @param thread 线程对象
     * @param detach 是否是分离对象 分离表示执行完毕自动销毁
     * @return
     */
    static bool create_thread(OSThread *thread, bool detach = true);

    /**
     * 等待线程
     * @param thread
     */
    static void join_thread(OSThread *thread);

    /**
     * ----------------
     * 内存
     * ----------------
     */
    /**
     * 获取页框的大小
     * @return
     */
    static int32_t page_size();

    /**
     * 获取全部的页框的个数
     * @return
     */
    static long total_pages();

    /**
     * 获取可用的页框的个数
     * @return
     */
    static long avail_pages();

    /**
     * 保留虚拟地址空间
     * @param F 内存的标记
     * @param bytes 申请的内存大小
     * @param fd 映射到的文件描述符，-1表示匿名映射
     * @return 保留空间的首地址
     */
    static void *reserve_memory(MEMFLAG F, size_t bytes, int32_t fd = -1);

    /**
     * 保留虚拟地址空间
     * @param F 内存的标记
     * @param bytes 申请的内存大小
     * @param align_bytes 对齐粒度，必须是页框的整倍数
     * @param fd 映射到的文件描述符，-1表示匿名映射
     * @return 保留空间的首地址
     */
    static void *reserve_memory_aligned(MEMFLAG F, size_t bytes, size_t align_bytes, int32_t fd = -1);

    /**
     * 保留虚拟地址空间，并且指定具体的地址
     * @param F 内存的标记
     * @param addr 需要保留到的内存首地址
     * @param bytes 申请的内存大小
     * @param fd 映射到的文件描述符，-1表示匿名映射
     * @param force 是否强制到这个地址，false 当之前已经申请会返回null,true会强制在此申请，可能会导致意外错误
     * @return 保留空间的首地址
     */
    static void *reserve_memory_at(MEMFLAG F, void *addr, size_t bytes, int32_t fd = -1, bool force = false);

    /**
     * 释放内存
     * @param F 内存的标记
     * @param addr 需要释放的内存首地址
     * @param bytes 申请的内存大小
     * @return 操作是否成功
     */
    static bool release_memory(MEMFLAG F, void *addr, size_t bytes);

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
    static bool commit_memory(MEMFLAG F,
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
    static bool uncommit_memory(MEMFLAG F,
                                void *addr,
                                size_t bytes);


    /**
     * @param start 虚拟地址起始位置
     * @param bytes 虚拟地址空间大小
     */
    static void pretouch_memory(void *start, size_t bytes);

    /**
     * 内存的dump
     * @param stream 目的输出流
     * @param addr 虚拟进程地址
     * @param bytes 虚拟进程地址空间长度
     * @param unit_bytes 每组显示的字节数
     * @param per_line_bytes 每行显示的字节数
     */
    static void dump_memory(CharOStream *stream,
                            void *addr,
                            size_t bytes,
                            int32_t unit_bytes = 2,
                            int32_t per_line_bytes = 16);
};

#endif // POSEI_OS_HPP