//
// Created by aurora on 2022/12/27.
//

#include "kernel/metaspace/CommittedLimiter.hpp"
#include "global/flag.hpp"

namespace metaspace {
    volatile size_t CommittedLimiter::_global_committed_bytes = 0;
    CALCUATE_COMMITTED_BYTES_FUNC CommittedLimiter::_policy_func = nullptr;

    size_t CommittedLimiter::possible_expand_bytes() {
        //在最大元空间限制下 允许的扩展的大小
        const auto committed_bytes = CommittedLimiter::committed_bytes();
        const auto bytes_below_max = global::MaxMetaspaceSize - committed_bytes;
        if(CommittedLimiter::_policy_func == nullptr){
            return bytes_below_max;
        }
        //在GC算法下允许的扩展的大小
        const auto bytes_below_policy =
                CommittedLimiter::_policy_func(committed_bytes);
        //取二者的小者
        return MIN2(bytes_below_max, bytes_below_policy);
    }
}