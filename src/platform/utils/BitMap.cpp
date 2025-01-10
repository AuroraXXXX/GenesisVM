//
// Created by aurora on 2023/9/19.
//

#include "kernel/utils/BitMap.hpp"
#include <cstring>
#include "plat/stream/CharOStream.hpp"
#include "plat/utils/OrderAccess.hpp"
#include "plat/utils/Bit.hpp"

bool BitMap::at(size_t no) const {
    assert(this->is_within(no), "索引序号错误");
    //获取bit 偏移量
    auto off = BitMap::word_offset_in_word(no);
    //获取制定字的指针
    auto word_ptr = this->word_addr(no);
    return Bit::bit_is_set_nth(*word_ptr, off);
}

size_t BitMap::count_range(size_t beg_no, size_t end_no) const {
    assert(::is_clamp(end_no, beg_no, this->_total_bits), "参数值错误");
    auto beg_full_word_index = BitMap::word_index_align_up(beg_no);
    auto end_full_word_index = BitMap::word_index_align_down(end_no);
    size_t sum = 0;

    if (beg_full_word_index < end_full_word_index) {
        //超过1个整个word
        sum += this->count_range_within_word(beg_no, BitMap::word_index_bit_no(beg_full_word_index));
        sum += this->count_range_full_word(beg_full_word_index, end_full_word_index);
        sum += this->count_range_within_word(BitMap::word_index_bit_no(end_full_word_index), end_no);
    } else {
        auto boundary = MIN2(BitMap::word_index_bit_no(beg_full_word_index), end_no);
        sum += this->count_range_within_word(beg_no, boundary);
        sum += this->count_range_within_word(boundary, end_no);
    }
    return sum;
}

void BitMap::set_range(size_t beg_no, size_t end_no) {
    assert(::is_clamp(end_no, beg_no, this->_total_bits), "参数值错误");
    auto beg_full_word_index = BitMap::word_index_align_up(beg_no);
    auto end_full_word_index = BitMap::word_index_align_down(end_no);
    if (beg_full_word_index < end_full_word_index) {
        //超过1个整个word
        this->set_range_within_word(beg_no, BitMap::word_index_bit_no(beg_full_word_index));
        this->set_range_full_word(beg_full_word_index, end_full_word_index);
        this->set_range_within_word(BitMap::word_index_bit_no(end_full_word_index), end_no);
    } else {
        auto boundary = MIN2(BitMap::word_index_bit_no(beg_full_word_index), end_no);
        this->set_range_within_word(beg_no, boundary);
        this->set_range_within_word(boundary, end_no);
    }
}

void BitMap::clear_range(size_t beg_no, size_t end_no) {
    assert(::is_clamp(end_no, beg_no, this->_total_bits), "参数值错误");
    auto beg_full_word_index = BitMap::word_index_align_up(beg_no);
    auto end_full_word_index = BitMap::word_index_align_down(end_no);
    if (beg_full_word_index < end_full_word_index) {
        //超过1个整个word
        this->clear_range_within_word(beg_no, BitMap::word_index_bit_no(beg_full_word_index));
        this->clear_range_full_word(beg_full_word_index, end_full_word_index);
        this->clear_range_within_word(BitMap::word_index_bit_no(end_full_word_index), end_no);
    } else {
        auto boundary = MIN2(BitMap::word_index_bit_no(beg_full_word_index), end_no);
        this->clear_range_within_word(beg_no, boundary);
        this->clear_range_within_word(boundary, end_no);
    }
}

void BitMap::print_stat(CharOStream *stream,
                                char one_char,
                                char zero_char,
                                int num_of_line) const {
    assert(one_char != zero_char, "无法区分字符");
    stream->print("   ");
    for (size_t i = 0; i < num_of_line; ++i) {
        stream->print("%01X ", i);
    }
    stream->cr();
    for (size_t i = 0; i < this->_total_bits; ++i) {
        if (i % num_of_line == 0) {
            stream->print("%02x ", i);
        }
        stream->print("%c ", this->at(i) ? one_char : zero_char);
        if ((i + 1) % num_of_line == 0) {
            stream->cr();
        }
    }

    stream->cr();
}

bool BitMap::par_change_through_mask(volatile bm_word_t *word_addr, bm_word_t mask) {
    //先获取一次旧的值
    auto old_val = OrderAccess::load<bm_word_t>(word_addr);
    do {
        //根据旧的值计算出新的值
        auto new_val = old_val & mask;
        //如果2次值相同 就说明之前已经有其他线程修改过了
        if (old_val == new_val) {
            return false;
        }
        //尝试使用1次CAS进行修改
        auto cur_val =
                OrderAccess::cas<bm_word_t>(word_addr, old_val, new_val);
        //如果原子修改前的值 和 旧值相同 那么就说明CAS成功了
        if (old_val == cur_val) {
            return true;
        }
        //CAS失败了 相当于读取1次最新的值 尝试再次进行修改
        old_val = cur_val;
    } while (true);
}

bool BitMap::par_set_bit(size_t bit_no) {
    assert(bit_no < this->total_bits(), "index is out of boundary");

    //先获取所在字的地址 使用volatile表示所指向的值是不确定的 不许使用缓存
    volatile auto *word_addr = this->word_addr(bit_no);
    //获取bitno在字上的偏移量
    auto offset = BitMap::word_offset_in_word(bit_no);
    //采用CAS将对应比特位设置成1
    const auto mask = (bm_word_t) 1 << offset;
    return par_change_through_mask(word_addr, mask);
}

bool BitMap::par_clear_bit(size_t bit_no) {
    assert(bit_no < this->total_bits(), "index is out of boundary");
    //先获取所在字的地址 使用volatile表示所指向的值是不确定的 不许使用缓存
    volatile auto *word_addr = this->word_addr(bit_no);
    //获取bitno在字上的偏移量
    auto offset = BitMap::word_offset_in_word(bit_no);
    //采用CAS将对应比特位设置成1
    const auto mask = ~((bm_word_t) 1 << offset);
    return par_change_through_mask(word_addr, mask);
}

size_t BitMap::count_range_within_word(size_t beg_no, size_t end_no) const {
    assert(::is_clamp(end_no, beg_no, this->total_bits()), "index is out of boundary");
    if (beg_no == end_no) {
        return 0;
    }
    auto mask = BitMap::bit_mask(beg_no, end_no);
    auto value = mask & *word_addr(beg_no);
    return Bit::count_total_ones(value);
}

size_t BitMap::count_range_full_word(size_t beg_index, size_t end_index) const {
    size_t sum = 0;
    auto word = this->_map + beg_index;
    auto end_word = this->_map + end_index;
    while (word < end_word) {
        auto value = *word++;
        sum += Bit::count_total_ones(value);
    }

    return sum;
}

void BitMap::set_range_within_word(size_t beg_no, size_t end_no) {
    assert(::is_clamp(end_no, beg_no, this->total_bits()), "index is out of boundary");
    if (beg_no != end_no) {
        *this->word_addr(beg_no) |= BitMap::bit_mask(beg_no, end_no);
    }
}

void BitMap::clear_range_within_word(size_t beg_no, size_t end_no) {
    assert(::is_clamp(end_no, beg_no, this->total_bits()), "index is out of boundary");
    if (beg_no != end_no) {
        *this->word_addr(beg_no) &= ~BitMap::bit_mask(beg_no, end_no);
    }
}

bool BitMap::par_at(size_t bit_no) const {
    assert(this->is_within(bit_no), "索引序号错误");
    //获取bit 偏移量
    auto off = BitMap::word_offset_in_word(bit_no);
    //获取制定字的指针
    auto word_ptr = this->word_addr(bit_no);
    auto value = OrderAccess::load(word_ptr);
    return Bit::bit_is_set_nth(value, off);
}


