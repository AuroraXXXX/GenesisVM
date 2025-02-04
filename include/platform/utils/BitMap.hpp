//
// Created by aurora on 2023/9/19.
//

#ifndef KERNEL_BIT_MAP_HPP
#define KERNEL_BIT_MAP_HPP

#include "platform/constants.hpp"
#include "platform/allocation.hpp"
#include <concepts>

#include "cstring"
#include "platform/macro.hpp"
#include "platform/utils/robust.hpp"
#include "platform/utils/align.hpp"
#include <atomic>

class BitView {
public:
    /**
     * bit map using type
     */
    using bm_t = uint64_t;
private:
    /**
     * bit map type total bits's log2
     */
    constexpr inline static auto LOG_BITS_PER_T = LogBitsPerByte * sizeof(bm_t);
    constexpr inline static auto BITS_PER_T = (decltype(LOG_BITS_PER_T)) 1 << LOG_BITS_PER_T;
protected:

    /**
     * 获取bit所在字的序号，向下对齐
     * @param bit_no bit序号
     * @return
     */
    static inline size_t bm_index_align_down(size_t bit_no) {
        return bit_no >> LOG_BITS_PER_T;
    };

    /**
     * 获取bit所在字的序号，向上对齐
     * @param bit_no bit序号
     * @return
     */
    static inline size_t bm_index_align_up(size_t bit_no) {
        return bm_index_align_down(bit_no + BITS_PER_T - 1);
    };

    /**
     * 获取字起始的bit序号
     * @param word_index
     * @return
     */
    static inline auto bit_no(size_t word_index) {
        return word_index << LogBitsPerWord;
    };

    /**
     * 生成 mask
     * @param start 生成的mask靠近
     * @param beg_no 开始的位置
     * @param end_no 结束的位置
     * @return
     */
    static bm_t many_bit_mask(bool start, size_t beg_no = 0, size_t end_no = BITS_PER_T);

    /**
     * 生成mask
     * @param bit_no 1的位置
     * @return
     */
    static inline auto one_bit_mask(size_t bit_no) {
        bit_no = offset_align(bit_no, BITS_PER_T);
        return (bm_t) 1 << bit_no;
    }

    /**
     * 将非atomic值转换成atomic类型
     * @param index
     * @param map
     * @return
     */
    static inline auto bm_ref(size_t bit_no, bm_t *map) {
        const auto index = bm_index_align_down(bit_no);
        return std::atomic_ref<bm_t>(map[index]);
    };

};

/**
 * 位图
 *
 * 用于 统计一段内存情况，这段内存也叫做统计区间
 *
 */
class BitMap : public BitView {

private:

private:
    /**
     * 统计区间内有效的比特位的个数
     */
    size_t _total_bits;
    /**
     * 统计区间的起始位置
     */
    bm_t *_map;
    NONCOPYABLE(BitMap);

public:
    /**
     * 有效的比特位的位数
     * @return
     */
    [[nodiscard]] inline auto total_bits() const {
        return this->_total_bits;
    };
protected:
    inline auto map() {
        return this->_map;
    };

    /**
     * 更新统计使用的内存
     * @param map 内存首地址
     * @param total_bits 实际需要使用的比特位数量
     */
    inline void update_map(void *map, size_t total_bits) {
        this->_total_bits = total_bits;
        this->_map = reinterpret_cast<bm_t *>(map);
    };
public:
    /**
     * 判断是否在统计范围内
     * @param bit_no 比特位序号
     * @return
     */
    [[nodiscard]] inline bool is_within(size_t bit_no) const {
        return bit_no < this->_total_bits;
    };

    /**
     * 判断此位置是否被标记
     * @param bit_no 比特位序号
     * @return
     */
    [[nodiscard]] bool at(size_t bit_no) const;


    /**
     * 统计[beg_no,end_no)区间的标记
     * @param beg_no 比特位开始序号
     * @param end_no 比特位结束序号
     */
    [[nodiscard]] size_t count_range(size_t beg_no, size_t end_no) const;

    /**
     * 设置[beg_no,end_no)区间的标记
     * @param beg_no 比特位开始序号
     * @param end_no 比特位结束序号
     */
    void set_range(size_t beg, size_t end);

    /**
     * 清除[beg_no,end_no)区间的标记
     * @param beg_no 比特位开始序号
     * @param end_no 比特位结束序号
     */
    void clear_range(size_t beg, size_t end);

    /**
    * 并发的设置标记
    * @param bit_no 比特位序号
    * @return 操作是否成功
    */
    bool par_set_bit(size_t bit_no);

    /**
     * 并发的清除标记
     * @param bit_no 比特位序号
     * @return 操作是否成功
     */
    bool par_clear_bit(size_t bit_no);

    [[nodiscard]] bool par_at(size_t bit_no) const;

    /**
     * 打印信息
     * @param stream 输出流
     * @param one_char 设置标记的字符
     * @param zero_char 未设置标记的字符
     * @param num_of_line 一行输出的个数
     */
    void print_stat(CharOStream *stream,
                    char one_char = 'X',
                    char zero_char = '-',
                    int num_of_line = 16) const;

    /**
     * 构造函数
     * @param base 比特内存的首地址
     * @param total_bits 上面的内存中有效的比特位个数
     */
    inline explicit BitMap(void *base, size_t total_bits) noexcept:
            _map(static_cast<bm_t *const>(base)),
            _total_bits(total_bits) {};

    inline explicit BitMap() noexcept: _map(nullptr), _total_bits(0) {};

    /**
     * 计算所需的字节数
     * @param bits 所需的比特位数量
     * @return
     */
    inline static size_t calc_bytes(size_t bits) {
        return BitMap::bm_index_align_up(bits) * sizeof(BitView::bm_t);
    };
};


/**
 * 支持调整bit map的功能
 * void *alloc(size_t bytes);
 * void free(void *ptr);
 * @tparam T
 */
template<typename T>
class GrowableBitMap : public BitMap {
private:
    NONCOPYABLE(GrowableBitMap);

protected:
    inline  explicit GrowableBitMap(void *map, size_t total_bits) :
            BitMap(map, total_bits) {};


public:
    inline explicit GrowableBitMap() : GrowableBitMap(nullptr, 0) {};

    void resize(size_t new_bits, bool clear = true);

    inline ~GrowableBitMap() {
        this->resize(0);
    };
};

/**
 * 直接内存
 */
class CHeapBitMap : public GrowableBitMap<CHeapBitMap> {
private:
    const MEMFLAG _flag;
public:
    explicit CHeapBitMap(MEMFLAG flag) :
            _flag(flag),
            GrowableBitMap() {};

    ~CHeapBitMap() {
        this->resize(0);
    }

    inline void *alloc(size_t bytes) {
        return NEW_CHEAP_ARRAY(char, bytes, this->_flag);
    };

    inline void free(void *ptr) {
        FREE_CHEAP_ARRAY(ptr, this->_flag);
    };
};


//--------------
template<typename T>
void GrowableBitMap<T>::resize(size_t new_bits, bool clear) {
    auto old_bytes = BitMap::calc_bytes(this->total_bits());
    auto new_bytes = BitMap::calc_bytes(new_bits);
    //获取内存申请和释放的对象
    auto derived = static_cast<T *>(this);
    if (old_bytes != new_bytes) {
        //需要释放陈旧的内存
        derived->free(this->map());
        if (new_bytes > 0) {
            auto map = derived->alloc(new_bytes);
            this->update_map(map, new_bits);
        } else {
            this->update_map(nullptr, 0);
        }

    }
    if (clear && this->map() != nullptr) {
        this->clear_range(0, new_bits);
    }
}


#endif //KERNEL_BIT_MAP_HPP