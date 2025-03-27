//
// Created by aurora on 2022/12/28.
//
#include "Segment.hpp"
#include "meta_log.hpp"
#include "LevelSegmentArray.hpp"
#include "platform/concurrent/Mutex.hpp"
#include "metaspace/Metaspace.hpp"
#define LOG_FMT         "SegmentMgr @" PTR_FORMAT
#define LOG_FMT_ARGS    this
namespace metaspace {
    /**
     * 获取对应内存块级别的 数组
     * @param list
     * @param min_committed_bytes
     * @return
     */
    static Segment *find_first_min_committed( LevelSegmentArray::Node *list, size_t min_committed_bytes) {
        Segment *target = nullptr;
        auto find_func = [&](Segment *segment) {
            if (segment->committed_bytes() >= min_committed_bytes) {
                target = segment;
                return false;
            }
            return true;
        };

        list->head_do(find_func);
        return target;
    }

    Segment *LevelSegmentArray::search_segment_descending(SegmentLevel_t level,
                                                       size_t min_committed_bytes) {
        const auto lowest_value = SegmentLevel::LV_LOWEST;
        for (auto lev = level; lev >= lowest_value; --lev) {
            auto segment_list = this->list_for_level(lev);
            auto target = find_first_min_committed(segment_list, min_committed_bytes);
            if (target != nullptr) {
                this->remove(target);
                return target;
            }
        }
        return nullptr;
    }

    Segment *LevelSegmentArray::search_segment_ascending(SegmentLevel_t level,
                                                         SegmentLevel_t max_level,
                                                      size_t min_committed_bytes) {
        for (auto lev = level; lev <= max_level; lev += 1 ) {
            auto segment_list = this->list_for_level(lev);

            auto target = find_first_min_committed(segment_list, min_committed_bytes);
            if (target) {
                //找到了
                this->remove(target);
                return target;
            }
        }
        return nullptr;
    }

    LevelSegmentArray::LevelSegmentArray() :
            _list_array(),
            _num_segments_at_level{} {
        for (size_t &a: this->_num_segments_at_level) {
            a = 0;
        }
    }

    size_t LevelSegmentArray::num_segments() const {
        size_t sum = 0;
        for (auto ele: this->_num_segments_at_level) {
            sum += ele;
        }
        return sum;
    }

    size_t LevelSegmentArray::total_bytes() const {
        size_t total_bytes = 0;
        for (SegmentLevel_t i = SegmentLevel::LV_LOWEST; i <= SegmentLevel::LV_HIGHEST;i++) {
            total_bytes += this->_num_segments_at_level[i] * SegmentLevel::get_bytes(i);
        }
        return total_bytes;
    }

    size_t LevelSegmentArray::calculate_committed_bytes_at_level(SegmentLevel_t level) const {
        assert(SegmentLevel::is_valid(level), "segment level is invalid");
        size_t committed_bytes = 0;
        auto calcu_func = [&](Segment *segment) {
            auto cur_committed_bytes =  segment->committed_bytes();
            committed_bytes +=cur_committed_bytes;
            //如果是0 那么说明之后也不存在已提交的内存
            return cur_committed_bytes != 0;
        };
        auto value = this->list_for_level(level);
        value->head_do(calcu_func);
        return committed_bytes;
    }

    void LevelSegmentArray::print_on(CharOStream *out) const {
        MutexLocker fcl(Metaspace::locker());
        out->print_cr(LOG_FMT ": 总计: %d Segment," SIZE_FORMAT " bytes.",
                      LOG_FMT_ARGS,
                      this->num_segments(),
                      this->total_bytes());

        for (SegmentLevel_t i = SegmentLevel::LV_LOWEST; i < SegmentLevel::LV_HIGHEST; i += 1) {
            out->print("-- List[" SEGMENT_LV_FORMAT "]:", i);
            auto list = this->list_for_level(i);
            if (list->is_empty()) {
                out->print_cr("null");
                continue;
            }
            auto list_print_func = [&](Segment *segment) {
                out->print(" - <");
                segment->print_on(out);
                out->print(">");
                return true;
            };
            this->list_for_level(i)->head_do(list_print_func);
            out->print_cr("- 总计: %d 块.", this->_num_segments_at_level[i]);
        }
    }

    void LevelSegmentArray::add(Segment *segment) {
        assert(segment != nullptr, "must be not null");
        auto list = this->list_for_level(segment->level());
        Segment *insert_target = nullptr;
        auto find_func = [&](Segment *node) {
            if (node->committed_bytes() >= segment->committed_bytes()) {
                insert_target = node;
                return false;
            }
            return true;
        };
        list->head_do(find_func);
        list->add( segment,insert_target, false);
    }

    void LevelSegmentArray::remove(Segment *segment) {
        auto list = this->list_for_level(segment->level());
        list->remove(segment);
    }

    bool LevelSegmentArray::contain(Segment *segment) {
        if (segment == nullptr) {
            return false;
        }
        const auto level = segment->level();
        if (!SegmentLevel::is_valid(level)) {
            return false;
        }
        auto list = this->list_for_level(level);
        return list->contain(segment);
    }


}