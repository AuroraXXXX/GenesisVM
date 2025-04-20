//
// Created by aurora on 2025/4/7.
//

#ifndef GENESISVM_COMMITTEDLIMITER_HPP
#define GENESISVM_COMMITTEDLIMITER_HPP

#include "platform/allocation.hpp"
#include <atomic>
namespace metaspace {
    using ALLOW_EXPAND_FUNC_T = bool(*)();
    class CommittedLimiter : public AllStatic {
        friend class Volume;

    private:
        static std::atomic<size_t> _committed_bytes;

        static ALLOW_EXPAND_FUNC_T _allow_expand_func;
    public:
        static inline size_t committed_bytes() {
            return _committed_bytes.load();
        };
        static inline void set_allow_expand_func(ALLOW_EXPAND_FUNC_T func) {
            _allow_expand_func = func;

        }
    };
};


#endif //GENESISVM_COMMITTEDLIMITER_HPP
