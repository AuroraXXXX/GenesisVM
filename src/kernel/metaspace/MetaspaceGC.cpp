//
// Created by aurora on 2024/2/10.
//

#include "MetaspaceGC.hpp"
#include "kernel/metaspace/CommittedLimiter.hpp"
#include "ContextHolder.hpp"
#include "global/flag.hpp"
#include "meta_log.hpp"
#include "plat/utils/align.hpp"

volatile size_t MetaspaceGC::_gc_threshold = 0;
uint32_t MetaspaceGC::_shrink_factor = 0;

void MetaspaceGC::compute_new_gc_threshold() {
    /**
     * used_after_gc 表示GC完成后已使用内存的 大小
     *
     * 对used_after_gc使用committed_bytes()是一种过高的估计，
     * 因为那时的空闲块列表包含在committed_bytes()中，
     * 而未分片的空闲块列表中的内存可用于未来的分配。
     * 但是，如果空闲块列表变得碎片化，那么内存可能无法用于未来的分配，因此内存是“正在使用”的。因此，
     * 在“使用中”的定义中包含空闲块列表是必要的。
     * 不包含空闲块列表会导致capacity_until_GC缩小到committed_bytes()以下，这在过去导致了严重的错误。
     */
    const auto context = metaspace::ContextHolder::context();
    const auto used_after_gc = (double) context->committed_bytes();
    //获取过去的GC阈值
    const auto gc_threshold = OrderAccess::load(&_gc_threshold);
    //日志信息
    log_trace(gc, metaspace)("MetaspaceGC::compute_new_gc_threshold:");
    log_trace(gc, metaspace)("   GC后已使用: %6.1fKB 元空间(metaspace)GC阈值: %6.1fKB",
                             used_after_gc / (double) K,
                             (double) gc_threshold / (double) K);
    /**
     * 在GC阈值调整的时候 优先考虑
     */
    if (MetaspaceGC::consider_expand_threshold(used_after_gc, gc_threshold)) {
        return;
    }
    //考虑缩容
    MetaspaceGC::consider_shrink_threshold(used_after_gc, gc_threshold);
    /**
     * 表明计算元空间新的阈值 结束了
     */
    log_trace(gc, metaspace)("MetaspaceGCCommitLimiter::compute_new_gc_threshold: end ");
}


bool MetaspaceGC::consider_expand_threshold(
        double used_after_gc,
        size_t gc_threshold) {
    assert(global::MinMetaspaceFreeRatio <= 100, "健全");
    //计算最小空闲比例
    const double min_free_per = double(global::MinMetaspaceFreeRatio) / 100.0;
    //那么最大使用比例 也就可以计算出来
    const double max_used_per = 1 - min_free_per;
    /**
     * 通过 used_after_gc 和 max_used_per 就可以得到希望的下一次GC阈值
     * 按照最小空闲比例进行计算出最小的GC阈值
     * 但实际这个数值必须应该设置在MetaspaceSize和MaxMetaspaceSize之间
     */
    auto min_desired_gc_threshold = clamp<size_t>(
            (size_t) (used_after_gc / max_used_per),
            global::MetaspaceSize,
            global::MaxMetaspaceSize);
    /**
     * 打印 计算出的信息
     */
    log_trace(gc, metaspace)("  最小空闲比例: %6.2f%% 最大使用比例: %6.2f%% ",
                             min_free_per,
                             max_used_per);
    if (gc_threshold >= min_desired_gc_threshold) {
        /**
         * 说明当前的GC阈值 >= 我们计算出的希望的最小GC阈值
         * 那么我们是不需要进行扩展GC阈值的
         */
        return false;
    }
    /**
     * 说明现在的GC阈值 < 我们的希望的GC阈值
     * 我们是需要扩展GC的阈值到达我们希望的大小
     */
    size_t expand_bytes = min_desired_gc_threshold - gc_threshold;
    expand_bytes = align_up(expand_bytes, metaspace::CommitGranuleBytes);
    /**
     * 超过最小限制才会实际增加
     */
    if (expand_bytes >= global::MinMetaspaceExpansion) {
        size_t new_gc_threshold = gc_threshold + expand_bytes;
        OrderAccess::store(&_gc_threshold, new_gc_threshold);
        log_trace(gc, metaspace)("  正在扩展.最小希望阈值: %6.1fKB 扩展: %6.1fKB "
                                 " MinMetaspaceExpansion: %6.1fKB"
                                 " 新的元空间(metaspace)GC阈值: %6.1fKB",
                                 (double) min_desired_gc_threshold / (double) K,
                                 (double) expand_bytes / (double) K,
                                 (double) global::MinMetaspaceExpansion / (double) K,
                                 (double) new_gc_threshold / (double) K);
    } else {
        log_trace(gc, metaspace)("   正在扩展失败.扩展: %6.1fKB,"
                                 "未达到 MinMetaspaceExpansion: %6.1fKB.",
                                 (double) expand_bytes / (double) K,
                                 (double) global::MinMetaspaceExpansion / (double) K);
    }
    return true;
}

bool MetaspaceGC::consider_shrink_threshold(
        double used_after_gc,
        size_t gc_threshold) {
    /**
     * 到了这里说明GC阈值 比按照最小空闲比例计算出的值还要大
     * 那么极有可能我们需要缩容 但是缩容的条件首先是设置了最大空闲比例
     */
    if (global::MaxMetaspaceFreeRatio >= 100) {
        return false;
    }
    /**
     * 计算出 最大的空闲比例 以及 最小使用比例
     */
    const double max_free_per = (double) global::MaxMetaspaceFreeRatio / 100.0;
    const double min_used_per = 1 - max_free_per;
    /**
     * 计算出希望的 最大的GC阈值
     */
    auto max_desired_gc_threshold = (size_t) clamp<double>(used_after_gc / min_used_per,
                                                           (double) global::MetaspaceSize,
                                                           (double) global::MaxMetaspaceSize);
    log_trace(gc, metaspace)("   最大空闲比例: %6.2f%% 最小使用比例: %6.2f%%",
                             max_free_per, min_used_per);
    if (gc_threshold <= max_desired_gc_threshold) {
        /**
         * 说明GC阈值 <= 最大的希望阈值 那么说明这个是我们允许的
         */
        return false;
    }
    /**
     * 下面需要计算缩小的大小
     *
     * 但是我们不希望发生 缩小后又需要增大阈值
     * 所以我们需要进行抑制缩减
     * 以确保我们是实际上需要缩减的
     *
     * 第一次调用 缩小  0%
     * 第二次调用 缩小 10%
     * 第三次调用 缩小 40%
     * 第四次调用 缩小100%
     */
    auto current_shrink_factor = MetaspaceGC::_shrink_factor;
    auto shrink_bytes = gc_threshold - max_desired_gc_threshold;
    shrink_bytes = shrink_bytes / 100 * current_shrink_factor;
    //但是缩减的字节数必须按照字节数对齐
    shrink_bytes = align_down(shrink_bytes, metaspace::CommitGranuleBytes);
    /**
     * 对缩小因子进行设置
     */
    if (current_shrink_factor == 0) {
        MetaspaceGC::_shrink_factor = 10;
    } else {
        MetaspaceGC::_shrink_factor = MIN2(current_shrink_factor * 4, 100u);
    }
    log_trace(gc, metaspace)("   缩减:当前因子:%d%% 新的缩减因子:%d%%",
                             current_shrink_factor, _shrink_factor);
    /**
     * 没有满足最小的扩展的大小 但是也应算作缩减成功
     */
    if (shrink_bytes < global::MinMetaspaceExpansion) {
        return true;
    }
    auto new_gc_threshold = gc_threshold - shrink_bytes;
    OrderAccess::store(&_gc_threshold, new_gc_threshold);
    log_trace(gc, metaspace)("  正在缩减: 最大希望阈值:%6.1fKB 缩减:%6.1fKB "
                             " MinMetaspaceExpansion:%6.1fKB"
                             " 新的元空间(metaspace)GC阈值: %6.1fKB",
                             (double) max_desired_gc_threshold / (double) K,
                             (double) shrink_bytes / (double) K,
                             (double) global::MinMetaspaceExpansion / (double) K,
                             (double) new_gc_threshold / (double) K);
    return true;
}


void MetaspaceGC::post_initialize() {
    auto context = metaspace::ContextHolder::context();
    MetaspaceGC::_gc_threshold = MAX2(global::MetaspaceSize,
                                      context->committed_bytes());

}

size_t MetaspaceGC::allowed_expansion(size_t committed_bytes) {
    size_t gc_threshold = OrderAccess::load(&_gc_threshold);
    assert(gc_threshold <= global::MaxMetaspaceSize, "GC阈值设置非法");
    assert(committed_bytes <= gc_threshold,
           "committed_bytes:" SIZE_FORMAT ",gc_threshold:",
           committed_bytes,
           gc_threshold);
    auto allowed_bytes = gc_threshold - committed_bytes;
    log_trace(metaspace)("MetaspaceGC:已允许扩展:" SIZE_FORMAT " bytes", allowed_bytes);
    return allowed_bytes;
}

void MetaspaceGC::global_initialize() {
    MetaspaceGC::_gc_threshold = global::MaxMetaspaceSize;
}

bool MetaspaceGC::threshold_with_gc(size_t bytes) {
    bytes = align_up(bytes, metaspace::CommitGranuleBytes);
    //
    bytes += global::MinMetaspaceExpansion;
    auto real_delta = clamp(bytes, global::MinMetaspaceExpansion, global::MaxMetaspaceExpansion);
    size_t old_gc_threshold = OrderAccess::load<>(&MetaspaceGC::_gc_threshold);
    auto new_gc_threshold = old_gc_threshold + real_delta;
    if (new_gc_threshold < old_gc_threshold) {
        // overhead
        new_gc_threshold = align_down(UINT64_MAX, metaspace::CommitGranuleBytes);
    }
    if (new_gc_threshold > global::MaxMetaspaceSize) {
        //无需重试 因为超过了，无法重试
        return false;
    }
    size_t prev_value = MetaspaceGC::gc_threshold_cas(
            old_gc_threshold,
            new_gc_threshold);
    auto need_retry = prev_value != old_gc_threshold;
    if (!need_retry) {
        //此时不需要重试 表示修改已经成功了
        log_trace(metaspace)("GC threshold increased :" SIZE_FORMAT "->" SIZE_FORMAT".",
                             old_gc_threshold,
                             new_gc_threshold);
    }
    return need_retry;
}


