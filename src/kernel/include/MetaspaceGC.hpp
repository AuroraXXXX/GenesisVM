//
// Created by aurora on 2024/2/10.
//

#ifndef NUCLEUSVM_METASPACEGC_HPP
#define NUCLEUSVM_METASPACEGC_HPP

#include "plat/mem/AllStatic.hpp"
#include "stdtype.hpp"
#include "plat/utils/OrderAccess.hpp"
/**
 * GC的阈值设置
 * 达到这个阈值 会导致内存申请失败 然后会进行GC
 */
class MetaspaceGC : public AllStatic {
private:
    /**
     * 越过此值会导致触发GC
     */
    static volatile size_t _gc_threshold;
    /**
     * 缩小的时候的 缩小比例
     */
    static uint32_t _shrink_factor;


    /**
     * 考虑 扩展内存
     * @param used_after_gc
     * @param gc_threshold
     * @return 是否需要进行扩展阈值
     */
    static bool consider_expand_threshold(
            double used_after_gc,
            size_t gc_threshold);

    /**
     * 考虑缩减内存
     * 但是必须 设置了最大空闲内存比例
     * @param used_after_gc
     * @param gc_threshold
     * @return
     */
    static bool consider_shrink_threshold(
            double used_after_gc,
            size_t gc_threshold);


public:
    /**
     * 计算GC新的阈值
     * 但是这个函数应该在安全点 整个虚拟机完成垃圾回收之后
     * 进行参数设置 整个时候只有一个线程在运行
     *
     * 缩小的条件
     * 设置了最大空闲比例
     */
    static void compute_new_gc_threshold();

    /**
     * 在元空间实际初始化时候调用
     * 将GC阈值调整到最大元空间内存值
     * 因为元空间实际初始化过程中 我们无法进行GC
     * 此时内存不足时会直接退出虚拟机
     */
    static void global_initialize();

    /**
     * 当实际初始化完成后进行调用
     * 将GC的阈值调整到合适的大小
     */
    static void post_initialize();

    /**
     * 在GC阈值下 还允许扩展的字节数
     * @param committed_bytes 元空间已提交字节数
     * @return 单位 字节
     */
    static size_t allowed_expansion(size_t committed_bytes);

    static inline auto gc_threshold_cas(
            size_t old_gc_threshold,
            size_t new_gc_threshold) {
        return OrderAccess::cas(
                &MetaspaceGC::_gc_threshold,
                old_gc_threshold,
                new_gc_threshold);
    };

    /**
     * GC时候 调整阈值
     * @param bytes 申请的字节数
     * @return 是否需要重试
     */
    static bool threshold_with_gc(size_t bytes);
};


#endif //NUCLEUSVM_METASPACEGC_HPP
