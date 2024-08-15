//
// Created by aurora on 2024/2/13.
//

#ifndef NUCLEUSVM_METASPACEARENA_HPP
#define NUCLEUSVM_METASPACEARENA_HPP

#include "stdtype.hpp"

namespace metaspace {
    class Arena;
}
class Mutex;


enum class MetaspaceType {
    Boot,
    Standard
};

class MetaspaceArena {
    friend class Metaspace;

private:
    metaspace::Arena *_arena;
    Mutex *const _mutex;
public:
    explicit MetaspaceArena(
            MetaspaceType space_type,
            Mutex *lock);

    ~MetaspaceArena();

    void *allocate(size_t bytes);

    void deallocate(void *ptr, size_t bytes);

    void usage_numbers(size_t *used_bytes,
                       size_t *committed_bytes,
                       size_t *capacity_bytes);
};


#endif //NUCLEUSVM_METASPACEARENA_HPP
