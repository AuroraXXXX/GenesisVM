//
// Created by aurora on 2025/3/27.
//
#include "platform/mem/ArenaMark.hpp"
#include "platform/mem/Arena.hpp"
#include "platform/utils/robust.hpp"
ArenaMark::ArenaMark(Arena* arena) :
        _current_top(arena->_list.peek()),
        _total_bytes(arena->_total_bytes),
        _end_literal(arena->_end_literal),
        _top_literal(arena->_top_literal) {
    assert(arena != nullptr, "must be");
    DEBUG_MODE_ONLY(this->_arena = arena;)
}

void ArenaMark::rollback_to(Arena* arena) {
    DEBUG_MODE_ONLY(assert(arena == this->_arena, "must be");)
    arena->_top_literal = this->_top_literal;
    arena->_end_literal = this->_end_literal;
    auto need_free_bytes = arena->_total_bytes - this->_total_bytes;
    arena->_total_bytes = this->_total_bytes;
    auto  total_bytes= arena->chop_list(this->_current_top);
    assert(need_free_bytes == total_bytes,"check");
}



