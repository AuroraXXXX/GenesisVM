//
// Created by aurora on 2022/12/23.
//

#include "Metaspace.hpp"
#include "meta_log.hpp"
#include "kernel/metaspace/constants.hpp"
#include "ContextHolder.hpp"
#include "SegmentHeaderPool.hpp"
#include "MetaspaceGC.hpp"
#include "global/flag.hpp"
#include "kernel/metaspace/CommittedLimiter.hpp"

#include "kernel/metaspace/Arena.hpp"
#include "kernel_mutex.hpp"

/**
 * 参数设置规范
 *
 * 元空间的参数
 * MaxMetaspaceSize是强制性的
 * MetaspaceSize是初始的大小
 * 这二者的参数是约束提交内存的大小但是无法约束保留的地址空间大小
 */
void metaspace::Metaspace::ergo_initialize() {
    meta_log_stream(info);
    metaspace::print_on_using_constants_setting(&log);

    const auto commit_granule_bytes = metaspace::CommitGranuleBytes;
    /**
     * 元空间参数
     * MaxMetaspaceSize
     * MetaspaceSize
     * 用于约束元空间提交内存 即实际内存的使用情况
     * 所以必须与提交粒度对齐
     */

    global::MaxMetaspaceSize = MAX2<size_t>(global::MaxMetaspaceSize, commit_granule_bytes);
    //调整参数
    if (global::MetaspaceSize > global::MaxMetaspaceSize) {
        global::MetaspaceSize = global::MaxMetaspaceSize;
    }
    /**
     * 进行向下对齐 确保不超过原本参数的要求
     * 但是至少满足一个提交粒度
     */
    global::MetaspaceSize = align_down_bounded(global::MetaspaceSize, commit_granule_bytes);
    global::MaxMetaspaceSize = align_down_bounded(global::MaxMetaspaceSize, commit_granule_bytes);
    assert(global::MetaspaceSize <= global::MaxMetaspaceSize, "健全");
    /**
     * 调整GC阈值
     */
    global::MaxMetaspaceFreeRatio = MIN2(100ul, global::MaxMetaspaceFreeRatio);
    global::MinMetaspaceFreeRatio = MIN2(100ul, global::MinMetaspaceFreeRatio);
    /**
     * 调整每次扩容和缩容的大小
     * 这个值应与提交粒度对齐
     */
    global::MaxMetaspaceExpansion = align_down_bounded(
            global::MaxMetaspaceExpansion,
            commit_granule_bytes);
    global::MinMetaspaceExpansion = align_down_bounded(
            global::MinMetaspaceExpansion,
            commit_granule_bytes);

}

void metaspace::Metaspace::global_initialize() {
    //1 应先初始化Metaspace
    metaspace::CommittedLimiter::register_policy(MetaspaceGC::allowed_expansion);
    MetaspaceGC::global_initialize();
    //2 初始化内存块头部
    metaspace::SegmentHeaderPool::initialize();
    metaspace::ContextHolder::init_context();
    log_info(metaspace)("[元空间模块]初始化完成.");
}

void metaspace::Metaspace::purge() {
    metaspace::ContextHolder::context()->purge();
}

void metaspace::Metaspace::post_initialize() {
    MetaspaceGC::post_initialize();
}
//
//void print_human_flag(const char *name, const size_t val, OutputStream *out) {
//    char unit = ' ';
//    auto format_val = val;
//    if (val >= G && val % G == 0) {
//        unit = 'G';
//        format_val = val / G;
//    } else if (val >= M && val % M == 0) {
//        unit = 'M';
//        format_val = val / M;
//    } else if (val >= K && val % K == 0) {
//        unit = 'K';
//        format_val = val / K;
//    }
//    out->print("[flag] %s : ", name);
//    out->print_cr(SIZE_FORMAT "%c", format_val, unit);
//}


void metaspace::Metaspace::print_on(CharOStream *out) {
    const auto context = metaspace::ContextHolder::context();
    out->print_raw(" [metaspace]:used ");
    out->print_human_bytes(context->used_bytes());
    out->print_raw(",committed ");
    out->print_human_bytes(context->committed_bytes());
    out->print_raw(",reserved ");
    out->print_human_bytes(context->reserved_bytes());
    out->print_cr(".");
}


//void *metaspace::Metaspace::expand_allocate_with_gc(MetaspaceArena *arena, size_t bytes) {
//    assert_locked_or_safepoint(Metaspace_lock);
//    do {
//        auto need_retry = MetaspaceGC::threshold_with_gc(bytes);
//        auto res = arena->_arena->allocate(bytes);
//        if (res != nullptr || !need_retry) {
//            return res;
//        }
//    } while (true);
//}