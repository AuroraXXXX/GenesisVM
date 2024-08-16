//
// Created by aurora on 2024/3/19.
//

#ifndef NUCLEUSVM_GROWABLEARRAY_HPP
#define NUCLEUSVM_GROWABLEARRAY_HPP

#include "plat/mem/allocation.hpp"
#include "plat/utils/robust.hpp"
#include "plat/utils/align.hpp"

/**
 * 固定长度的数组
 * @tparam E
 */
template<typename E>
class FixedArray {
protected:
    E *_data;
    /**
     * 可使用的长度
     */
    int32_t _len;
    /**
     * 最大容量
     */
    int32_t _capacity;

    explicit FixedArray() :
            _len(0),
            _data(nullptr),
            _capacity(0) {
    }

public:
    explicit FixedArray(E *data, int32_t capacity, int32_t len);

    inline void at_put(int32_t index, E &value) const {
        assert(is_clamp<>(index, 0, this->_len), "illegal index");
        return this->_data[index] = value;
    };

    inline E &at(int32_t index) const {
        assert(is_clamp<>(index, 0, this->_len), "illegal index");
        return this->_data[index];
    };

    inline E *addr_at(int32_t index) const {
        assert(is_clamp(index, 0, this->_len), "illegal index");
        return this->_data + index;
    };

    inline E pop() {
        assert(_len > 0, "empty");
        return this->_data[--this->_len];
    };

    inline E &top() const {
        assert(_len > 0, "empty");
        return this->_data[this->_len - 1];
    };

    bool push(E &e);

    void remove_at(int index);
};


/**
 * 支持可扩展长度的功能，注意底层使用拷贝方式实现
 * @tparam E
 * @tparam Derived
 */
template<typename E>
class GrowableArray : public FixedArray<E> {
private:
    /**
     * 1 NativeMemory   MEMFLAG << 1 | 1
     * 2 Resource        0
     * 3 Arena          存放的是地址 | 0
     */
    uintptr_t _meta;
protected:
    void expand_to(int32_t new_capacity);

    [[nodiscard]] inline bool on_arena() const {
        return (this->_meta & 1) == 0 && this->_meta != 0;
    };

    [[nodiscard]] inline bool on_native() const {
        return (this->_meta & 1) == 1;
    };

    [[nodiscard]] inline bool on_resource() const {
        return this->_meta == 0;
    };

    [[nodiscard]] inline MEMFLAG mem_flag() const {
        return MEMFLAG(this->_meta >> 1);
    };

    [[nodiscard]] inline Arena *arena() const {
        return (Arena *) (this->_meta);
    };
public:
    void append(E &e);

    /**
     * 使用Resource进行分配
     * @param init_capacity
     */
    explicit GrowableArray(int32_t init_capacity = 16);

    /**
     * 使用指定的Arena进行内存分配
     * @param arena
     * @param init_capacity
     */
    explicit GrowableArray(Arena *arena, int32_t init_capacity);

    /**
     * 使用NativeMemory进行内存分配
     * @param F
     * @param init_capacity
     */
    explicit GrowableArray(MEMFLAG F, int32_t init_capacity);

    ~GrowableArray();
};
/**
 * --------------------------
 * implements
 * --------------------------
 */
template<typename E>
void FixedArray<E>::remove_at(int index) {
    assert(0 <= index && index < _len, "illegal index");
    for (int j = index + 1; j < _len; j++) {
        _data[j - 1] = _data[j];
    }
    this->_len--;
}

template<typename E>
bool FixedArray<E>::push(E &e) {
    if (this->_len == this->_capacity) return false;
    this->_data[this->_len++] = e;
    return true;
}

template<typename E>
FixedArray<E>::FixedArray(E *data, int32_t capacity, int32_t len):
        _capacity(capacity),
        _len(len),
        _data(data) {}

template<typename E>
void GrowableArray<E>::expand_to(int32_t new_capacity) {
    auto old_capacity = this->_capacity;
    assert(new_capacity > old_capacity && old_capacity >= 0,
           "expected growth but %d <= %d", new_capacity, old_capacity);
    this->_capacity = new_capacity;
    //根据元数据的类型不同 从不同地方创建数组
    E *new_data;
    if (this->on_resource()) {
        new_data = NEW_RESOURCE_ARRAY(E, new_capacity);
    } else if (this->on_native()) {
        new_data = NEW_CHEAP_ARRAY(E, new_capacity, this->mem_flag());
    } else {
        assert(this->on_arena(), "unknown meta value");
        new_data = NEW_ARENA_ARRAY(this->arena(), E, new_capacity);
    }
    //将旧的数据拷贝到新的上面
    memcpy(new_data, this->_data, sizeof(E) * this->_len);
    //看看是否有必要释放内存
    if (this->_data != nullptr && this->on_native()) {
        FREE_CHEAP_ARRAY(this->_data, this->mem_flag());
    }
    //更新指针
    this->_data = new_data;
}

template<typename E>
GrowableArray<E>::GrowableArray(int32_t init_capacity):
        FixedArray<E>(),
        _meta(0) {
    this->expand_to(init_capacity);
}

template<typename E>
GrowableArray<E>::~GrowableArray() {
    if (this->on_native()) {
        FREE_CHEAP_ARRAY(this->_data, this->mem_flag());
    }
}

template<typename E>
GrowableArray<E>::GrowableArray(Arena *arena, int32_t init_capacity):
        _meta((uintptr_t) arena),
        FixedArray<E>() {
    assert((this->_meta & 1) == 0 && arena != nullptr, "must be");
    this->expand_to(init_capacity);
}

template<typename E>
GrowableArray<E>::GrowableArray(MEMFLAG F, int32_t init_capacity):
        _meta(uintptr_t(F) << 1 | 1), FixedArray<E>() {
    assert(F != MEMFLAG::None, "must be");
    this->expand_to(init_capacity);
}

template<typename E>
void GrowableArray<E>::append(E &e) {
    if (this->_len == this->_capacity) {
        assert(this->_capacity <= INT32_MAX,"OVERFLOW");
        auto new_capacity = (int32_t)round_up_power_of_2((uint32_t)this->_capacity);
        assert(new_capacity <= INT32_MAX,"overflow");
        this->expand_to(new_capacity);
    }
    auto success = this->push(e);
    assert(success,"must be success.");
}
#endif //NUCLEUSVM_GROWABLEARRAY_HPP
