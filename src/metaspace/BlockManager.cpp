//
// Created by aurora on 2022/12/16.
//

#include "BlockManager.hpp"

namespace metaspace {
    void *BlockManager::allocate(size_t requested_bytes, size_t *real_bytes) {
        assert(requested_bytes >= BlockManager::MIN_BYTES,
               "请求的内存大小过小(" SIZE_FORMAT")", requested_bytes);
        void *p = nullptr;
        if (requested_bytes >= BlockTree::MIN_BYTES) {
            p = this->_tree.remove_meta_node(requested_bytes, real_bytes);
        } else {
            p = (void *) this->_small_blocks.remove_node(requested_bytes, real_bytes);
        }
        if (p) {
            const size_t waste_bytes = *real_bytes - requested_bytes;
            /**
             * 把浪费掉的内存放入到对应的
             */
            if (waste_bytes >= BlockManager::MIN_BYTES) {
                const auto ptr = (void *) ((uintptr_t) p + requested_bytes);
                this->deallocate(ptr, waste_bytes);
            }
        }
        return p;
    }

    void BlockManager::deallocate(void *p, size_t bytes) {
        assert(bytes >= BlockManager::MIN_BYTES,
               "回收的内存大小过小(" SIZE_FORMAT")", bytes);
        if (bytes >= BlockTree::MIN_BYTES) {
            this->_tree.add_meta_node(p, bytes);
        } else {
            this->_small_blocks.add_node(p, bytes);
        }
    }


}