//
// Created by aurora on 2025/4/7.
//

#include "CommittedLimiter.hpp"
std::atomic<size_t > metaspace::CommittedLimiter::_committed_bytes = 0;
metaspace::ALLOW_EXPAND_FUNC_T metaspace::CommittedLimiter::_allow_expand_func = nullptr;