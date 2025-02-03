//
// Created by aurora on 2023/9/19.
//

#include "platform/utils/BitMap.hpp"
#include <cstring>
#include "platform/stream/CharOStream.hpp"
#include "platform/utils/robust.hpp"
#include <bit>

bool BitMap::at(size_t no) const {
    assert(this->is_within(no), "索引序号错误");
    auto index = BitView::bm_index_align_down(no);
    auto value = this->_map[index];
    //获取bit mask
    auto mask = BitView::mask(no);
    return (value & mask) != 0;
}

size_t BitMap::count_range(size_t beg_no, size_t end_no) const {
    assert(::is_clamp(end_no, beg_no, this->_total_bits), "参数值错误");
    auto beg_full_word_index = BitMap::bm_index_align_down(beg_no);
    auto end_full_word_index = BitMap::bm_index_align_up(end_no);
    size_t sum = 0;
    if (beg_full_word_index < end_full_word_index) {
        //超过1个整个word
        auto value = BitView::mask(true, beg_no) & this->_map[beg_full_word_index++];
        sum += std::popcount(value);
        value = BitView::mask(false, 0, end_no) & this->_map[end_full_word_index];
        sum += std::popcount(value);
        while (beg_full_word_index < end_full_word_index) {
            sum += std::popcount(this->_map[beg_full_word_index++]);
        }
    } else {
        auto value = BitView::mask(true, beg_no, end_no) & this->_map[beg_full_word_index];
        sum += std::popcount(value);
    }
    return sum;
}

void BitMap::set_range(size_t beg_no, size_t end_no) {
    assert(::is_clamp(end_no, beg_no, this->_total_bits), "参数值错误");
    auto beg_full_word_index = BitMap::bm_index_align_down(beg_no);
    auto end_full_word_index = BitMap::bm_index_align_up(end_no);
    if (beg_full_word_index < end_full_word_index) {
        //超过1个整个word
        this->_map[beg_full_word_index++] |= BitView::mask(true, beg_no);
        this->_map[end_full_word_index] |= BitView::mask(false, 0, end_no);
        auto total_bytes = (end_full_word_index - beg_full_word_index) * sizeof(bm_t);
        ::memset(this->_map + beg_full_word_index, 0xFF, total_bytes);
    } else {
        this->_map[beg_full_word_index] |= BitView::mask(true, beg_no, end_no);
    }
}

void BitMap::clear_range(size_t beg_no, size_t end_no) {
    assert(::is_clamp(end_no, beg_no, this->_total_bits), "参数值错误");
    auto beg_full_word_index = BitMap::bm_index_align_down(beg_no);
    auto end_full_word_index = BitMap::bm_index_align_up(end_no);
    if (beg_full_word_index < end_full_word_index) {
        //超过1个整个word
        this->_map[beg_full_word_index++] &= ~BitView::mask(true, beg_no);
        this->_map[end_full_word_index] &= ~BitView::mask(false, 0, end_no);
        auto total_bytes = (end_full_word_index - beg_full_word_index) * sizeof(bm_t);
        ::memset(this->_map + beg_full_word_index, 0x00, total_bytes);
    } else {
        //没有超过一个bm_t
        this->_map[beg_full_word_index] &= ~BitView::mask(true, beg_no, end_no);
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

bool BitMap::par_set_bit(size_t bit_no) {
    assert(bit_no < this->total_bits(), "index is out of boundary");

    //先获取所在字的地址 使用volatile表示所指向的值是不确定的 不许使用缓存
    auto ref = BitView::bm_ref(bit_no, this->_map);
    //获取bitno在字上的mask
    auto mask = BitView::mask(bit_no);
    auto old_value = ref.fetch_or(mask);
    return (old_value & mask) == 0;
}

bool BitMap::par_clear_bit(size_t bit_no) {
    assert(bit_no < this->total_bits(), "index is out of boundary");
    //先获取所在字的地址 使用volatile表示所指向的值是不确定的 不许使用缓存
    auto ref = BitView::bm_ref(bit_no, this->_map);
    //获取bitno在字上的mask
    auto mask = BitView::mask(bit_no);
    auto old_value = ref.fetch_and(~mask);
    return (old_value & mask) == 0;
}


bool BitMap::par_at(size_t bit_no) const {
    assert(this->is_within(bit_no), "索引序号错误");
    //先获取所在字的地址 使用volatile表示所指向的值是不确定的 不许使用缓存
    auto value = BitView::bm_ref(bit_no, this->_map).load();
    //获取bitno在字上的mask
    auto mask = BitView::mask(bit_no);
    return (value & mask) != 0;
}

auto BitView::mask(bool start, size_t beg_no, size_t end_no) {

    // 获取内部的偏移量
    beg_no = offset_align<size_t>(beg_no, BITS_PER_T);
    end_no = offset_align<size_t>(end_no, BITS_PER_T);
    // 获取mask的宽度
    const auto width = end_no - beg_no;
    // 获取 移位 的偏移
    const auto offset = start ? BITS_PER_T - width : 0;
    return generate_mask<bm_t>(width, offset);

}
