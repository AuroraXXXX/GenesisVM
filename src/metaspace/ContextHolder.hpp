//
// Created by aurora on 2025/3/25.
//

#ifndef GENESISVM_CONTEXTHOLDER_HPP
#define GENESISVM_CONTEXTHOLDER_HPP
#include "platform/mem/AllStatic.hpp"

namespace metaspace{
    class VolumeList;
    class LevelSegmentArray;
    class Segment;

    /**
     * 上下文管理类
     */
    class ContextHolder :public AllStatic{
    private:
      static  VolumeList* _volume_list;
        static LevelSegmentArray* _level_segment_array;
    public:
        /**
         * 归还 segment
         * @param segment
         */
        static void return_segment(Segment* segment);
    };
}


#endif //GENESISVM_CONTEXTHOLDER_HPP
