//
// Created by aurora on 2022/12/25.
//

#include "CommittedMask.hpp"
#include "plat/stream/CharOStream.hpp"
#include "plat/utils/align.hpp"
namespace metaspace {
    static inline void * addr_add(void * addr,size_t bytes){
        return (void *)((uintptr_t)addr + bytes);
    }
    size_t CommittedMask::get_committed_bytes_in_range(void* range_start,
                                                      size_t range_bytes) const {
        auto start_no = CommittedMask::bit_no_for_address(range_start);
        auto end_no =
                CommittedMask::bit_no_for_address(addr_add(range_start , range_bytes));
        return this->count_range(start_no, end_no) *
               CommittedMask::statistics_bytes_per_bit();
    }

    CommittedMask::CommittedMask(
            void *range_start,
            size_t range_bytes) :
            _base((uintptr_t)range_start),
            CHeapBitMap(MEMFLAG::Metaspace){
        assert_is_aligned(range_bytes, CommittedMask::statistics_bytes_per_bit());
        const auto total_effective_bit =
                range_bytes / CommittedMask::statistics_bytes_per_bit();
        this->resize(total_effective_bit, true);
    }


    void CommittedMask::mark_range_as_committed(void *range_start,
                                                size_t range_bytes) {
        auto beg = this->bit_no_for_address(range_start);
        auto end = this->bit_no_for_address(addr_add(range_start , range_bytes));
        this->set_range(beg, end);
    }

    void CommittedMask::mark_range_as_uncommitted(void *range_start,
                                                  size_t range_bytes) {
        auto beg = this->bit_no_for_address(range_start);
        auto end = this->bit_no_for_address(addr_add(range_start , range_bytes));
        this->clear_range(beg, end);
    }


    void CommittedMask::print_on(CharOStream *out,
                                 char committed_char,
                                 char uncommitted_char) const {
        assert(committed_char != uncommitted_char, "提交状态字符和未提交状态字符相同");
        out->print("commit mask:区间[" PTR_FORMAT "," PTR_FORMAT "):",
                   this->_base,
                   this->_base + this->mapping_range_bytes());
        this->print_stat(out, committed_char, uncommitted_char);
        out->cr();
    }

}