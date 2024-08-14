//
// Created by aurora on 2022/12/28.
//
#include "Segment.hpp"
#include "meta_log.hpp"
#include "SegmentManager.hpp"
#include "kernel_mutex.hpp"

#define LOG_FMT         "SegmentMgr @" PTR_FORMAT
#define LOG_FMT_ARGS    this
namespace metaspace {
    static Segment *find_first_min_committed(LinkList<Segment> *list, size_t min_committed_bytes) {
        Segment *target = nullptr;
        auto find_func = [&](Segment *segment) {
            if (segment->committed_bytes() >= min_committed_bytes) {
                target = segment;
                return false;
            }
            return true;
        };

            list->node_head_do(find_func);
        return target;
    }

    Segment *SegmentManager::search_segment_descending(SegmentLevel level,
                                                       size_t min_committed_bytes) {
        const auto lowset_value = (SegementLevel_t)SegmentLevel::LV_LOWEST;
        for (auto lev = (SegementLevel_t)level; lev >= lowset_value; --lev) {
            auto segment_list = this->list_for_level((SegmentLevel)lev);
            auto target = find_first_min_committed(segment_list, min_committed_bytes);
            if (target != nullptr) {
                this->remove(target);
                return target;
            }
        }
        return nullptr;
    }

    Segment *SegmentManager::search_segment_ascending(SegmentLevel level,
                                                      SegmentLevel max_level,
                                                      size_t min_committed_bytes) {
        for (auto lev = level; lev <= max_level; lev = (SegmentLevel)((SegementLevel_t)lev + 1)) {
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

    SegmentManager::SegmentManager() :
            _list_array(),
            _num_segments_at_level{} {
        for (size_t &a: this->_num_segments_at_level) {
            a = 0;
        }
    }

    size_t SegmentManager::num_segments() const {
        size_t sum = 0;
        for (auto ele: this->_num_segments_at_level) {
            sum += ele;
        }
        return sum;
    }

    size_t SegmentManager::total_bytes() const {
        size_t total_bytes = 0;
        for (SegmentLevel i = SegmentLevel::LV_LOWEST; i <= SegmentLevel::LV_HIGHEST;
            i = (SegmentLevel)((SegementLevel_t)i + 1)) {
            total_bytes += this->_num_segments_at_level[(SegementLevel_t)i] * level_to_bytes(i);
        }
        return total_bytes;
    }

    size_t SegmentManager::calculate_committed_bytes_at_level(SegmentLevel level) const {
        assert(level_is_valid(level), "segment level is invalid");
        size_t committed_bytes = 0;
        auto calcu_func = [&](Segment *segment) {
            committed_bytes += segment->committed_bytes();
            return true;
        };
        auto value = this->list_for_level(level);
        value->node_head_do(calcu_func);
        return committed_bytes;
    }

    void SegmentManager::print_on(CharOStream *out) const {
        MutexLocker fcl(Metaspace_lock);
        out->print_cr(LOG_FMT ": 总计: %d Segment," SIZE_FORMAT " bytes.",
                      LOG_FMT_ARGS,
                      this->num_segments(),
                      this->total_bytes());

        for (SegmentLevel i = SegmentLevel::LV_LOWEST; i < SegmentLevel::LV_HIGHEST; i = (SegmentLevel)((SegementLevel_t)i + 1)) {
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
            this->list_for_level(i)->node_head_do(list_print_func);
            out->print_cr("- 总计: %d 块.", this->_num_segments_at_level[(SegementLevel_t)i]);
        }
    }

    void SegmentManager::add(Segment *segment) {
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
        list->node_head_do(find_func);
        list->add_to_list_target(insert_target, segment, false);
    }

    void SegmentManager::remove(Segment *segment) {
        auto list = this->list_for_level(segment->level());
        list->delete_from_list(segment);
    }

    bool SegmentManager::contain(Segment *segment) {
        if (segment == nullptr) {
            return false;
        }
        const auto level = segment->level();
        if (!level_is_valid(level)) {
            return false;
        }
        auto list = this->list_for_level(level);
        return list->contain(segment);
    }


}