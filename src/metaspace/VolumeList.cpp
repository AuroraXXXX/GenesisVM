//
// Created by aurora on 2022/12/16.
//
#include "platform/os.hpp"
#include "platform/concurrent/Mutex.hpp"
#include "ContextHolder.hpp"
#include "VolumeList.hpp"
#include "Volume.hpp"
#include "meta_log.hpp"
#include "Setting.hpp"

#define LOG_FMT "VolumeList @" PTR_FORMAT
#define LOG_FMT_ARGS this
namespace metaspace {

    VolumeList::VolumeList() :
            _list(),
            _reserved_bytes(0),
            _committed_bytes(0) {
        meta_log(info, "born");
    }

    void VolumeList::create_new_volume() {
        //获取锁
        assert_lock_strong(ContextHolder::global_locker());
        //获取默认的虚拟节点大小
        const auto volume_bytes = Setting::VolumeDefaultBytes;
        //创建虚拟节点
        auto ptr = os::reserve_memory_aligned(MEMFLAG::Metaspace,
                                              volume_bytes,
                                              volume_bytes);
        //如果创建失败，抛出异常
        if (ptr == nullptr) {
            vm_exit_out_of_memory(VMErrorType::OOM_MMAP_ERROR,
                                  volume_bytes,
                                  "reserved volume bytes failed.");
        }
        //创建空间
        Space space(ptr, volume_bytes);
        //增加已分配的字节数
        this->_reserved_bytes += space.capacity_bytes();
        //创建虚拟节点
        auto volume = new Volume(space, &this->_committed_bytes);
        //将虚拟节点加入列表
        this->_list.push(volume);
    }

    VolumeList::~VolumeList() {
        assert_lock_strong(ContextHolder::global_locker());
        while (true) {
            auto vsn = this->_list.pop();
            if (vsn == nullptr) {
                break;
            }
            this->_reserved_bytes.fetch_sub(vsn->reserved_bytes());
            delete vsn;
        }
        meta_log(info, "dies");
    }


    void VolumeList::print_on(CharOStream *out) const {
        out->print_cr(LOG_FMT ":", LOG_FMT_ARGS);
        int n = 0;
        const auto iter_func = [&](Volume *volume) {
            out->print(" -node #%d:", n);
            volume->print_on(out);
            ++n;
            return true;
        };
        this->_list.iterate(iter_func);

        out->print_cr(" - 总计 %d 节点,reserved:" SIZE_FORMAT
                      " bytes,committed:" SIZE_FORMAT " bytes.",
                      n, this->reserved_bytes(), this->committed_bytes());
    }

    bool VolumeList::contains(void *p) const {
        bool result = false;
        auto iter_func = [&](Volume *volume) {
            result = volume->contain(p);
            //如果是包含的 就说明不需要继续往下寻找了
            return !result;
        };
        this->_list.iterate(iter_func);
        return result;
    }
    Segment *VolumeList::allocate_root_segment() {
        return nullptr;
    }
#ifdef DEBUG_MODE_ONLY
    void VolumeList::verify() {
        auto iter_func = [&](Volume *volume) {
            volume->verify();
            return true;
        };
        this->_list.iterate(iter_func);
    }



#endif

}
