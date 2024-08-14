//
// Created by aurora on 2022/12/16.
//
#include "plat/os/mem.hpp"
#include "VolumeList.hpp"
#include "Volume.hpp"
#include "kernel_mutex.hpp"
#include "meta_log.hpp"

#define LOG_FMT "VolumeList @" PTR_FORMAT
#define LOG_FMT_ARGS this
namespace metaspace {

    VolumeList::VolumeList() :
            _list_length(0),
            _reserved_bytes(0),
            _committed_bytes(0),
            _list_head(nullptr) {
        meta_log(debug, "出生(born)");
    }

    void VolumeList::create_new_volume() {
        assert_lock_strong(Metaspace_lock);
        //创建虚拟节点
        auto ptr = os::reserve_memory_aligned(MEMFLAG::Metaspace, VolumeDefaultBytes,VolumeDefaultBytes);
        if (ptr == nullptr) {
            vm_exit_out_of_memory(VMErrorType::OOM_MMAP_ERROR,
                                  VolumeDefaultBytes,
                                  "reserved volume bytes failed.");
        }
        Space space(ptr, VolumeDefaultBytes);
        this->_reserved_bytes += space.capacity_bytes();
        auto volume = new Volume(space, &this->_committed_bytes);
        volume->set_next(this->_list_head);
        this->_list_head = volume;
        //更新信息
        ++this->_list_length;
    }

    VolumeList::~VolumeList() {
        assert_lock_strong(Metaspace_lock);
        auto vsn = this->_list_head;
        Volume *vsn_next;
        while (vsn) {
            vsn_next = vsn->next();
            this->_reserved_bytes -= vsn->reserved_bytes();
            delete vsn;
            vsn = vsn_next;
        }
        meta_log(debug, "死亡(dies)");
    }

    Segment *VolumeList::allocate_root_segment() {
        if (this->_list_head == nullptr ||
            !this->_list_head->has_unused_region()) {
            this->create_new_volume();
            meta_log2(debug, "已添加新的虚拟节点(now:%d)", this->_list_length);
        }
        auto segment = this->_list_head->allocate_root_segment();
        assert(segment != nullptr, "必须不为空");
        return segment;
    }



    void VolumeList::print_on(CharOStream *out) const {
        MutexLocker fcl(Metaspace_lock);
        out->print_cr(LOG_FMT ":", LOG_FMT_ARGS);
        auto vsn = this->_list_head;
        int n = 0;
        while (vsn) {
            out->print(" -node #%d:", n);
            vsn->print_on(out);
            vsn = vsn->next();
            ++n;
        }
        out->print_cr(" - 总计 %d 节点,reserved(保留):" SIZE_FORMAT
                      " bytes,committed(提交):" SIZE_FORMAT " bytes.",
                      n, this->reserved_bytes(), this->committed_bytes());
    }

    bool VolumeList::contains(void* p) const {
        for (auto cur = this->_list_head; cur != nullptr; cur = cur->next()) {
            if (cur->contain(p)) {
                return true;
            }
        }
        return false;
    }
#ifdef DIAGNOSE
    void VolumeList::verify() {
        for (auto cur = this->_list_head; cur != nullptr; cur = cur->next()) {
            cur->verify();
        }
    }
#endif

}
