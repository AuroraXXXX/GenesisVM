//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_BLOCK_ARRAY_HPP
#define KERNEL_METASPACE_BLOCK_ARRAY_HPP

#include "stdtype.hpp"
#include "plat/utils/align.hpp"
#include "plat/utils/robust.hpp"
#include <new>

namespace metaspace {
    /**
     * 使用数组管理这些极小的内存
     *
     * @tparam MIN_BYTES 可以管理的最小内存大小
     * @tparam MAX_BYTES 可以管理的最大内存大小 但是这个内存不包括在内
     * @tparam ELEM_BYTES 管理内存的字节对齐大小
     */
    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    class BlockArray {
    public:
        /**
         * 数组的长度
         */
        constexpr inline static size_t LIST_LENGTH = (MAX_BYTES - MIN_BYTES) / ELEM_BYTES;
    private:
        /**
         * 被BinList管理的内存块 在内部统一转换成Node结构体 进行管理
         */
        struct Node {
        public:
            Node *_next;
        };

        /**
         * 确保整个程序可以良好运行的静态断言
         */
        static_assert(MIN_BYTES >= sizeof(Node));
        static_assert(MAX_BYTES > MIN_BYTES);
        static_assert(ELEM_BYTES > 0);
        static_assert((MAX_BYTES - MIN_BYTES) % ELEM_BYTES == 0);
        /**
         * 管理细小内存块的数组
         */
        Node *_bin_list[LIST_LENGTH];
        /**
         * 可以管理的总共字节数
         */
        size_t _total_bytes;

        /**
         * 将字节数转换成对应的索引
         * @param bytes 字节数
         * @return 索引号
         */
        static size_t bytes2index(size_t bytes);

        /**
         * 将索引转换成对应的字节数
         * @param index 索引号
         * @return
         */
        static size_t index2bytes(size_t index);

        /**
         * 寻找下一个不是空的索引
         * @param index 原始的索引
         * @return 新的索引
         *          LIST_LENGTH 表示没找到
         */
        size_t next_non_empty_index(size_t index);

    public:
        explicit BlockArray();

        /**
         * 添加内存块进行管理
         * @param p 内存块的首地址
         * @param bytes 内存块的大小
         */
        void add_node(void *p, size_t bytes);

        /**
         * 寻找 bytes内存块，
         * exact为true,表示精确模式查找.只要找不到就返回nullptr
         * exact为false,表示至少模式查找.如果找不到,
         *              会返回一个比要求大的内存块
         *              实际的大小会回填到bytes参数中
         * 会找更大的内存块
         * @param required_bytes 需求内存的字节数 应该字节对齐完毕
         * @param real_bytes 实际的大小会回填到bytes参数中
         * @param exact 是否是精确查找
         * @return 内存块大小 如果找不到则返回nullptr
         */
        void *remove_node(size_t required_bytes, size_t *real_bytes, bool exact = false);

        /**
         * 判断是否是空
         * @return
         */
        inline bool is_empty() { return this->_total_bytes == 0; };

        inline size_t total_bytes() { return this->_total_bytes; };
    };

    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    size_t BlockArray<MIN_BYTES, MAX_BYTES, ELEM_BYTES>::bytes2index(size_t bytes) {
        assert_is_aligned(bytes, ELEM_BYTES);
        auto index = (bytes - MIN_BYTES) / ELEM_BYTES;
        assert(index < LIST_LENGTH, "check index");
        return index;
    }

    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    size_t BlockArray<MIN_BYTES, MAX_BYTES, ELEM_BYTES>::index2bytes(size_t index) {
        assert(index < LIST_LENGTH, "错误的索引:%d", index);
        return MIN_BYTES + ELEM_BYTES * index;
    }

    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    BlockArray<MIN_BYTES, MAX_BYTES, ELEM_BYTES>::BlockArray()  :
            _total_bytes(0) {
        for (size_t i = 0; i < LIST_LENGTH; ++i) {
            this->_bin_list[i] = nullptr;
        }
    }

    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    size_t BlockArray<MIN_BYTES, MAX_BYTES, ELEM_BYTES>::next_non_empty_index(size_t index) {
        assert(index < LIST_LENGTH, "错误的索引:%d", index);
        size_t next_index = index;
        while (next_index < LIST_LENGTH && this->_bin_list[next_index] == nullptr) {
            next_index++;
        }
        return next_index;
    }

    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    void BlockArray<MIN_BYTES, MAX_BYTES, ELEM_BYTES>::add_node(void *p, size_t bytes) {
        bytes = align_up(bytes, ELEM_BYTES);
        assert(is_clamp(bytes, MIN_BYTES, MAX_BYTES), "内存块大小错误,无法被管理.");
        assert_is_aligned(bytes, ELEM_BYTES);
        const auto index = this->bytes2index(bytes);
        auto new_head = reinterpret_cast<Node *>(p);
        new_head->_next = this->_bin_list[index];
        this->_bin_list[index] = new_head;
        this->_total_bytes += bytes;
    }

    template<size_t MIN_BYTES, size_t MAX_BYTES, size_t ELEM_BYTES>
    void *BlockArray<MIN_BYTES, MAX_BYTES, ELEM_BYTES>::remove_node(
            size_t required_bytes,
            size_t *real_bytes,
            bool exact) {
        required_bytes = align_up(required_bytes, ELEM_BYTES);
        auto index = this->bytes2index(required_bytes);
        if (!exact) {
            index = this->next_non_empty_index(index);
        } else {
            //-1表示没找到不为空的
            index = this->_bin_list[index] != nullptr ? index : LIST_LENGTH;
        }
        //没找到
        if (index == LIST_LENGTH) {
            *real_bytes = 0;
            return nullptr;
        }
        //找到 进行处理
        auto p = this->_bin_list[index];
        assert(p != nullptr, "程序错误");
        //调整对应的节点
        this->_bin_list[index] = p->_next;
        //更新统计的信息
        auto node_bytes = this->index2bytes(index);
        this->_total_bytes -= node_bytes;
        *real_bytes = node_bytes;
        return p;
    }

}

#endif //KERNEL_METASPACE_BLOCK_ARRAY_HPP
