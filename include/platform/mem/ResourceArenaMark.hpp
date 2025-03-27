//
// Created by aurora on 2025/3/27.
//

#ifndef PLATFORM_RESOURCE_ARENA_MARK_HPP
#define PLATFORM_RESOURCE_ARENA_MARK_HPP
#include "platform/mem/ArenaMark.hpp"
/**
 * OSThread 的 resource mem
 */
class ResourceArenaMark : protected ArenaMark {

public:
     explicit ResourceArenaMark();

     ~ResourceArenaMark();
};
#endif //PLATFORM_RESOURCE_ARENA_MARK_HPP
