//
// Created by aurora on 2022/12/16.
//

#include "kernel/metaspace/Arena.hpp"

#include "kernel/metaspace/constants.hpp"
#include "BlockManager.hpp"
#include "Segment.hpp"
#include "ContextHolder.hpp"
#include "kernel/metaspace/InternalStats.hpp"
#include "meta_log.hpp"

#define LOG_FMT         "Arena @" PTR_FORMAT
#define LOG_FMT_ARGS    this
namespace metaspace {
    Arena::Arena(ArenaGrowthPolicy *policy) :
            _block_manager(nullptr),
            _num_of_segments(0),
            _policy(policy) {
        meta_log(debug, "出生(born)");
        InternalStats::inc_num_arena_births();
    }

    Arena::~Arena() {
        int count = 0;
        size_t total_bytes = 0;
        const auto cm = ContextHolder::context();

        auto deallocate_func = [&](Segment *segment) {
            total_bytes += segment->total_bytes();
            ++count;
            meta_log2(debug, "归还:" SEGMENT_FORMAT, SEGMENT_FORMAT_ARGS(segment));
            cm->return_segment(segment);
            return true;
        };
        this->_segments.node_head_do(deallocate_func);
        assert(count == this->_num_of_segments, "程序错误");
        {
            meta_log_stream(debug);
            log.print("已归还 %d segment,",
                      count);
            log.print_human_bytes(total_bytes);
            log.print_raw_cr(".");
        }
        //调整大小
        ContextHolder::context()->sub_arena_used_bytes(total_bytes);
        if (this->_block_manager) {
            delete this->_block_manager;
            this->_block_manager = nullptr;
        }
        meta_log(debug, "死亡(dies)");
        InternalStats::inc_num_arena_deaths();
    }

    bool Arena::attempt_enlarge_current_segment(size_t need_bytes) {
        const auto current = this->current_use_segment();
        //已经是根块了 就没办法在扩大合并了
        if (current->is_root_segment()) {
            return false;
        }
        //超过内存块可以表示的最大范围
        if (current->used_bytes() + need_bytes > RegionBytes) {
            return false;
        }
        //首先寻找新的内存块等级
        const auto new_level = bytes_to_level(
                current->used_bytes() + need_bytes);
        //新块的等级应该比当前块等级低 也就是说 比当前块大
        assert(new_level <= current->level(), "健全");
        //如果放大的倍数超过了2倍 那我们就放弃 ;放大2倍就可以解决99%问题
        if (new_level < (SegmentLevel) ((SegementLevel_t) current->level() - 1)) {
            return false;
        }
        //这是由于伙伴合并后空闲块在前面 我们无法在使用简单的指针碰撞进行分配
        // 所以只能当前块是leader 然后合并其他的块
        if (!current->is_leader()) {
            return false;
        }
        //如果扩展策略下一次的建议的内存块 还没有 添加的块大 那么就不要扩大了
        if (this->policy_suggest_next_level() > current->level()) {
            return false;
        }
        //下面要进行扩展内存了
        bool success = ContextHolder::context()->
                attempt_enlarge_segment(current);
        assert(!success || current->free_bytes() >= need_bytes, "健全");
        return success;
    }


    Segment *Arena::create_new_segment(size_t need_bytes) {
        /*
         * 每次申请不能超过根块大小的限制
         */
        guarantee(need_bytes <= RegionBytes,
                  "请求过大(" SIZE_FORMAT " byte)-每次申请最大允许" SIZE_FORMAT " byte",
                  need_bytes,
                  RegionBytes);
        //至少 满足要求的 内存块大小
        const auto max_level = bytes_to_level(need_bytes);
        /*
         * 希望的大小 :根据策略建议大小 和 至少的大小 ,取二者的最大值
         * 这里内存块等级越小 内存块越大
         */
        const auto preferred_level = MIN2(max_level,
                                          this->policy_suggest_next_level());
        auto segment = ContextHolder::context()
                ->get_segment(preferred_level,
                              max_level,
                              need_bytes);
        /**
         * 这些断言仅仅在获取到内存才判断
         */
        assert(!segment || segment->is_inuse(), "Segment状态错误");
        assert(!segment || segment->free_below_committed_bytes() >= need_bytes, "Segment没有被提交");
        return segment;
    }

    void Arena::salvage_segment(Segment *segment) {
        assert(segment != nullptr, "must not be null");
        auto remain_bytes = segment->free_below_committed_bytes();
        if (remain_bytes < BlockManager::MIN_BYTES) {
            return;
        }
        meta_log2(trace,
                  "正在回收当前 Segment 的剩余提交内存:" SEGMENT_FULL_FORMAT,
                  SEGMENT_FULL_FORMAT_ARGS(segment));
        auto p = segment->allocate(remain_bytes);
        //应该是把剩余所有的提交内存全部获取
        assert(p != nullptr && segment->free_below_committed_bytes() == 0, "健全");
        //更新统计的信息 由于申请后的内存仅仅放入到隶属于本类的BlockManager
        // 我们应该也认为这个内存被使用了
        ContextHolder::context()->add_arena_used_bytes(remain_bytes);
        this->_block_manager->deallocate(p, remain_bytes);
    }


    void Arena::deallocate(void *p, size_t bytes) {
        assert(this->current_use_segment() != nullptr, "非法的销毁");

        auto raw_bytes = get_raw_byte_for_requested(bytes);
        meta_log2(trace, "正在回收" PTR_FORMAT ",size:" SIZE_FORMAT " byte,实际:" SIZE_FORMAT " byte",
                  p, bytes, raw_bytes);
        if (this->_block_manager == nullptr) {
            this->_block_manager = new BlockManager();
        }
        this->_block_manager->deallocate(p, raw_bytes);
    }

    void *Arena::allocate(size_t required_bytes) {
        assert(required_bytes <= RegionBytes, "请求的字节数过大");
        auto raw_bytes = get_raw_byte_for_requested(required_bytes);
        meta_log2(trace, "请求:" SIZE_FORMAT "B,实际:" SIZE_FORMAT "B",
                  required_bytes, raw_bytes);
        /**
         * 首先从BlockManager中获取
         */
        auto p = this->allocate_from_block(raw_bytes);
        if (p) {
            return p;
        }
        p = this->allocate_from_current_segment(raw_bytes);

        /**
         * 当前块满足不了需求 那么需要申请一个新的内存块
         * 并从新的块中申请内存
         */
        if (p == nullptr) {
            p = this->allocate_from_new_segment(raw_bytes);
        }
        /**
         * 日志输出 以及相关的信息统计
         */
        if (p) {
            /**
             * 说明肯定是申请成功了
             * 更新统计信息
             */
            ContextHolder::context()->add_arena_used_bytes(raw_bytes);
            InternalStats::inc_num_allocs();
            meta_log2(trace, "申请后:%u segment,当前:" SEGMENT_FULL_FORMAT,
                      this->_num_of_segments,
                      SEGMENT_FULL_FORMAT_ARGS(this->current_use_segment()));
            meta_log2(trace, "新申请" SIZE_FORMAT "B:[" PTR_FORMAT "," PTR_FORMAT ")", required_bytes,
                      p, (void *) ((uintptr_t) p + raw_bytes));
        } else {
            //说明申请失败 达到了限制
            InternalStats::inc_num_allocs_failed_limit();
            meta_log(info, "申请失败,返回null");
        }
        return p;
    }

    void Arena::usage_numbers(size_t *used_bytes,
                              size_t *committed_bytes,
                              size_t *capacity_bytes) {
        size_t used = 0, committed = 0, capacity = 0;
        auto iter_func = [&](Segment *segment) {
            used += segment->used_bytes();
            committed += segment->committed_bytes();
            capacity += segment->total_bytes();
            return true;
        };
        this->_segments.node_head_do(iter_func);
        if (used_bytes)
            *used_bytes = used;
        if (committed_bytes)
            *committed_bytes = committed;
        if (capacity_bytes)
            *capacity_bytes = capacity;
    }

    void *Arena::allocate_from_block(size_t need_bytes) {
        if (this->_block_manager == nullptr || this->_block_manager->is_empty()) {
            return nullptr;
        }
        size_t real_bytes;
        const auto p = this->_block_manager->allocate(need_bytes, &real_bytes);
        if (p) {
            DEBUG_MODE_ONLY(InternalStats::inc_num_allocs_from_blocks_manager();)
            meta_log2(trace, "已从BlockManager申请:" SIZE_FORMAT " byte(实际:" SIZE_FORMAT
                    " byte).现在BlockManager: " SIZE_FORMAT " byte",
                      need_bytes,
                      real_bytes,
                      this->_block_manager->total_bytes());
        }
        return p;
    }

    void *Arena::allocate_from_current_segment(size_t need_bytes) {
        const auto current = this->current_use_segment();
        if (current == nullptr) {
            return nullptr;
        }
        bool current_too_small = false;
        bool commit_failure = false;

        void *p = nullptr;
        /**
         * 尝试从当前块申请 或者扩展当前块 然后尽可能满足用户需求进行分配
         */

        /**
         * 当前块空闲大小太小 无法满足我们的需求
         * 那么块中已提交的内存肯定也无法满足需求
         * 所以我们首先要尝试扩大当前的块
         */
        if (current->free_bytes() < need_bytes) {
            if (this->attempt_enlarge_current_segment(need_bytes)) {
                DEBUG_MODE_ONLY(InternalStats::inc_num_segments_enlarged();)
                meta_log(debug, "enlarged segment");
            } else {
                current_too_small = true;
            }
        }
        /**
         * 如果当前块不是太小 即 当前块的空闲空间 应该是满足要求的
         * 这是程序逻辑
         */
        assert(!current_too_small &&
               current->free_bytes() >= need_bytes, "健全");
        /**
         * 如果当前块空闲大小满足了需求
         * 那么我们就需要要求当前块担保已提交内存满足需求
         * 这里面可能会触发提交内存
         *
         * 担保失败时候 就说明内存提交失败了
         */
        if (!current_too_small &&
            !current->ensure_committed_enough_and_acquire_lock(need_bytes)) {
            //如果确保提交 失败了 那么极有可能达到限制 说明提交失败了
            meta_log2(info, "提交失败(需求:" SIZE_FORMAT " byte)", need_bytes);
            commit_failure = true;
        }
        /**
         * 二者错误均没有
         * 说明当前块空闲大小满足要求且已提交内存中也可以分配下
         * 我们再次尝试进行分配 且肯定会成功
         */
        if (!current_too_small && !commit_failure) {
            p = current->allocate(need_bytes);
            assert(p != nullptr, "从当前segment申请失败");
        }

        /**
         * 当没有从当前块申请到内存的时候
         * 肯定是这三者之一的问题
         * 1 当前块为空
         * 2 当前块太小了扩展失败
         * 3 内存提交失败了
         */
        assert(p == nullptr && (current == nullptr ||
                                current_too_small || commit_failure),
               "健全");
        return p;
    }

    void *Arena::allocate_from_new_segment(size_t need_bytes) {
        auto new_segment = this->create_new_segment(need_bytes);
        if (new_segment == nullptr) {
            meta_log2(info, "为了" SIZE_FORMAT
                    " byte而申请一个新的segment失败",
                      need_bytes);
            return nullptr;
        }
        meta_log2(debug, "已为" SIZE_FORMAT
                " byte而申请一个新的segment(" SEGMENT_FORMAT ")",
                  need_bytes, SEGMENT_FORMAT_ARGS(new_segment));
        assert(new_segment->free_below_committed_bytes() >= need_bytes, "健全");
        if (this->current_use_segment()) {
            //当前块存在，那么就需把当前块进行回收
            this->salvage_segment(this->current_use_segment());
            DEBUG_MODE_ONLY(InternalStats::inc_num_segments_retire();)
        }
        /**
         * 将新块插入到链表中
         * 形成新的链表
         */
        this->_segments.tail_add_to_list(new_segment);
        ++this->_num_of_segments;
        /**
         * 接下来我们需要从新的块中再次执行申请
         */
        auto p = new_segment->allocate(need_bytes);
        assert(p != nullptr, "健全");
        return p;
    }

    void Arena::print_on(CharOStream *stream) {
        size_t calc_used_bytes = 0;
        size_t calc_reserved_bytes = 0;
        size_t calc_committed_bytes = 0;
        this->usage_numbers(&calc_used_bytes,
                            &calc_committed_bytes,
                            &calc_reserved_bytes);
        stream->print_cr("arena : %d segments, total bytes: "
                         SIZE_FORMAT ", committed bytes: " SIZE_FORMAT,
                         this->_num_of_segments,
                         calc_reserved_bytes,
                         calc_committed_bytes);
        stream->cr();
        stream->print_cr("growth-policy " PTR_FORMAT ", block_manager " PTR_FORMAT,
                         this->_policy,
                         this->_block_manager);
    }


}
