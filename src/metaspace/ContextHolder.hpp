//
// Created by aurora on 2025/3/25.
//

#ifndef GENESISVM_CONTEXTHOLDER_HPP
#define GENESISVM_CONTEXTHOLDER_HPP
#include "platform/mem/AllStatic.hpp"

namespace metaspace{
    class VolumeList;
    class LevelSegmentArray;
    class ContextHolder :public AllStatic{
    private:
        VolumeList* _volume_list;
        LevelSegmentArray* _level_segment_array;
    public:

    };
}


#endif //GENESISVM_CONTEXTHOLDER_HPP
