//
// Created by aurora on 2022/12/16.
//
#include "Volume.hpp"
#include "Segment.hpp"
#include "CommittedBitMap.hpp"
#include "metaspace/InternalStats.hpp"
#include "meta_log.hpp"
#include "platform/os.hpp"
#include "Setting.hpp"
#include "platform/utils/align.hpp"
#include "metaspace/Metaspace.hpp"
#define LOG_FMT "Volume @" PTR_FORMAT " base=" PTR_FORMAT" "
#define LOG_FMT_ARGS this,this->_reserved.start()

namespace metaspace {
    Volume::Volume(Space &virtual_space,
                   std::atomic<size_t> *committed_statistics) :
            _next(nullptr),
            _reserved(virtual_space),

            _committed_statistics(committed_statistics),
            _commit_bitmap(virtual_space.start(),virtual_space.capacity_bytes()) {
        assert_is_aligned(virtual_space.capacity_bytes(), Setting::RegionBytes);

        //新增统计信息
        InternalStats::inc_num_volumes_births();
        meta_log2(debug, "出生(born),size " SIZE_FORMAT " K", this->reserved_bytes() / K);
    }

    Volume::~Volume() {

        auto reserved_bytes = this->reserved_bytes();
        meta_log2(debug, "死亡(dies),size " SIZE_FORMAT " K", reserved_bytes / K);
        auto committed_bytes = this->committed_bytes_slow_path();

       /**
         * 修改虚拟链表 内存提交的统计信息
         */
        this->_committed_statistics->fetch_sub(committed_bytes);
        /**
         * 修改内部运行状态的统计信息 用户检测
         */
        InternalStats::inc_num_volumes_deaths();
    }

    void Volume::print_on(CharOStream *out) {
        assert_lock_strong(Metaspace::locker());
        out->print(LOG_FMT, LOG_FMT_ARGS);
        out->print("reserved=");
        out->print_human_bytes(this->reserved_bytes());
        out->print(",committed=");
        out->print_human_bytes(this->committed_bytes_slow_path());
        out->print(",used=");

        /**
         * 打印内存内存提交情况
         */
        this->_commit_bitmap.print_on(out);
    }
#ifdef DEBUG_MODE_ONLY
    void Volume::verify() const {
        assert_lock_strong(Metaspace::locker());
        assert_is_aligned((size_t)this->_reserved.start(),
                                  this->_reserved.capacity_bytes());
        assert_is_aligned(this->reserved_bytes(),Setting::RegionBytes);
    }
#endif

}
