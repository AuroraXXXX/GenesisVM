//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_BLOCK_MANAGER_HPP
#define KERNEL_METASPACE_BLOCK_MANAGER_HPP

#include "constants.hpp"
#include "BlockArray.hpp"
#include "BlockTree.hpp"

namespace metaspace {
    class BlockManager {
    public:
        /**
         * BlockManager 可以管理的最小内存块
         * 分配和回收必须遵顼这个要求
         * 由于SmallArray决定
         */
        constexpr inline static size_t MIN_BYTES = BytesPerWord;
        /**
         * 用以管理非常小的内存块
         */
        typedef BlockArray<MIN_BYTES,
                BlockTree::MIN_BYTES,
                MetaAlignedBytes> SmallArray;
    private:
        SmallArray _small_blocks;
        BlockTree _tree;
    public:
        explicit BlockManager() : _small_blocks(), _tree() {};

        /**
         * 从当前的管理的细小内存块中非陪内存
         * @param requested_bytes 请求的大小
         * @param real_bytes 实际内存大小
         * @return 内存块首地址
         */
        void *allocate(size_t requested_bytes, size_t *real_bytes = nullptr);

        /**
         * 将内存块回收，进行管理
         * @param p 内存块首地址
         * @param bytes 内存块大小 单位字节
         */
        void deallocate(void *p, size_t bytes);

        /**
         * 管理的内存是否是空
         * @return
         */
        inline bool is_empty() {
            return this->_small_blocks.is_empty() && this->_tree.is_empty();
        };

        inline size_t total_bytes() {
            return this->_small_blocks.total_bytes() + this->_tree.total_bytes();
        };
    };

}
#endif //KERNEL_METASPACE_BLOCK_MANAGER_HPP
