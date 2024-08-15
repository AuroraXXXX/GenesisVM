//
// Created by aurora on 2022/12/27.
//

#ifndef KERNEL_METASPACE_COMMITTED_LIMITER_HPP
#define KERNEL_METASPACE_COMMITTED_LIMITER_HPP

#include "plat/mem/allocation.hpp"
#include "plat/mem/AllStatic.hpp"
#include "plat/utils/OrderAccess.hpp"

namespace metaspace {
   /**
    * 提交内存的限制器
    * @param bytes 目前已经提交的内存数
    * @return 允许扩充的字节数
    */
    using CALCUATE_COMMITTED_BYTES_FUNC = size_t (*)(size_t bytes);

    /**
     * 用于统计元空间全部的内存
     */
    class CommittedLimiter : public AllStatic {
    private:
        /**
         * 统计元空间所有使用的内存
         */
        static volatile size_t _global_committed_bytes;

        static CALCUATE_COMMITTED_BYTES_FUNC _policy_func;

        inline static auto committed_bytes() {
            return OrderAccess::load(&_global_committed_bytes);
        };
    public:
        /**
         * 可能的扩展的字节数
         * @return
         */
        static size_t possible_expand_bytes();

        static inline void register_policy(CALCUATE_COMMITTED_BYTES_FUNC policy_func) {
            OrderAccess::store(&_policy_func, policy_func);
            OrderAccess::compile_barrier();
        };

        static inline void increase_committed_bytes(size_t committed_bytes) {
            OrderAccess::fetch_and_add(&_global_committed_bytes, committed_bytes);
        };

        static inline void decrease_committed_bytes(size_t committed_bytes) {
            OrderAccess::fetch_and_sub(&_global_committed_bytes, committed_bytes);
        };

    };
}


#endif //KERNEL_METASPACE_COMMITTED_LIMITER_HPP
