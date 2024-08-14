//
// Created by aurora on 2022/12/16.
//

#ifndef KERNEL_METASPACE_SEGMENT_HPP
#define KERNEL_METASPACE_SEGMENT_HPP

#include "stdtype.hpp"
#include "kernel/metaspace/constants.hpp"
#include "plat/utils/align.hpp"

namespace metaspace {
    class Volume;

    /**
     * 内存块的有效负载(即覆盖的内存)可能已提交 部分提交 完全未提交
     *        +--------------+ <- end    -----------+ ----------+
     *        |              |                      |           |
     *        |              |                      |           |
     *        |              |                      |           |
     *        |              |                      |           |
     *        |              |                      |           |
     *        | -----------  | <- committed_top  -- +           |
     *        |              |                      |           |
     *        |              |                      | "free"    |
     *        |              |                      |           | size
     *        |              |     "free_below_     |           |
     *        |              |        committed"    |           |
     *        |              |                      |           |
     *        |              |                      |           |
     *        | -----------  | <- top     --------- + --------  |
     *        |              |                      |           |
     *        |              |     "used"           |           |
     *        |              |                      |           |
     *        +--------------+ <- start   ----------+ ----------+
     */
    template<typename T>
    class SegmentBase {
    private:
        /**
         * 通过前驱节点和后继节点将
         */
        T *_next;
        T *_prev;
        /**
         * 这两个指针是固定的
         * 指向地址空间分配时候 虚拟节点中MetaChunk的关系
         * 用于内存块的合并和切分
         */
        T *_prev_buddy;
        T *_next_buddy;
        /**
         * 隶属于的虚拟节点
         */
        Volume *_container;
    protected:
        inline void set_container(Volume *container) {
            this->_container = container;
        };
    public:
        explicit SegmentBase() :
                _next_buddy(nullptr),
                _prev_buddy(nullptr),
                _next(nullptr),
                _prev(nullptr),
                _container(nullptr) {};

        /**
         * --------------------------------
         * buddy node
         */
        inline T *prev_buddy() {
            return this->_prev_buddy;
        };

        inline T *next_buddy() {
            return this->_next_buddy;
        };

        inline void set_prev_buddy(T *segment) {
            this->_prev_buddy = segment;
        };

        inline void set_next_buddy(T *segment) {
            this->_next_buddy = segment;
        };

        /**
         * -------------------------
         *
         * @return
         */
        [[nodiscard]] inline T *next() const {
            return this->_next;
        };

        [[nodiscard]] inline T *prev() const {
            return this->_prev;
        };

        inline void set_next(T *segment) {
            this->_next = segment;
        };

        inline void set_prev(T *segment) {
            this->_prev = segment;
        };

        [[nodiscard]] inline Volume *container() const {
            return this->_container;
        };
    };

    class Segment : public SegmentBase<Segment> {
    private:
        /**
         * 管理的内存首地址
         */
        uintptr_t _base;
        /**
         * 提交的内存大小
         */
        size_t _committed_bytes;
        /**
         * 已经使用的内存大小
         */
        size_t _used_bytes;
        /**
         * 内存块等级
         */
        SegmentLevel _level;
        /**
         * 表示当前块的状态
         * InUse表示当前块在使用 已经分配或者部分被分配出去
         * Free 表示当前块被SpaceManager管理
         * Dead 表示被ChunkHeaderPool所管理，但是没有持有任何的内存
         *
         * 状态的调整
         * 当从
         */
        enum class State : uint8_t {
            InUse,
            Free,
            Dead
        };
        State _state;

        /**
         * 将内存边界向上调整
         * @param new_commit_bytes 新的提交内存边界
         * @return false 表示达到了限制
         */
        bool commit_up_to(size_t new_commit_bytes);

        /**
         * 确保[base,base + bytes)这个区间内存被提交
         * 若这个区间小于提交粒度 会向两侧对齐 满足提交粒度的大小
         * 然后进行提交 若这个内存粒度已经被提交完毕 那么不会有任何影响
         *
         * 若这个区间大于是提交粒度的N倍 那么由于不与其他内存块共享粒度
         * 对于其他内存块不会有影响
         * @param base 区间的首地址
         * @param bytes 区间的长度
         * @return
         */
        bool ensure_range_is_committed(void *base, size_t bytes);

    public:
        /**
         * 将所有数据进行擦除 除了状态字段
         */
        void clear();

        /**
         * 提供给ChunkHeaderPool
         */
        explicit Segment();

        /**
         * 用于初始化 主要是复用
         * 即在ChunkPoolHeader分配时候使用
         * @param container
         * @param start
         * @param level
         */
        void initialize(Volume *container, void *start, SegmentLevel level);

        [[nodiscard]] inline size_t total_bytes() const {
            return level_to_bytes(this->_level);
        };

        [[nodiscard]] inline void * base() const {
            return (void *)(this->_base);
        };

        [[nodiscard]] inline auto used_top() const {
            return (void *)(this->_base + this->_used_bytes);
        };

        [[nodiscard]] inline void *committed_top() const {
            return (void *)(this->_base + this->_committed_bytes);
        };

        [[nodiscard]] void *end() const {
            return (void *)(this->_base + this->total_bytes());
        };

        /**
         * 重置已使用的内存
         */
        inline void reset_used_top() {
            this->_used_bytes = 0;
        };


        /**
         * 已经使用的内存 不包括内存块的开销
         * @return
         */
        [[nodiscard]] inline size_t used_bytes() const {
            return this->_used_bytes;
        };

        [[nodiscard]] inline size_t committed_bytes() const {
            return this->_committed_bytes;
        };

        inline void set_committed_bytes(size_t committed_bytes) {
            this->_committed_bytes = committed_bytes;
        };

        [[nodiscard]] size_t free_bytes() const {
            return this->total_bytes() - this->used_bytes();
        };

        /**
         * 判断已经提交的内存中 剩余可用的内存
         * @return
         */
        [[nodiscard]] inline size_t free_below_committed_bytes() const {
            return this->_committed_bytes - this->_used_bytes;
        };

        /**
         * -------------
         * 对当前内存块MetaChunk状态的设置和获取
         */
        [[nodiscard]] inline bool is_free() const {
            return this->_state == State::Free;
        };

        [[nodiscard]] inline bool is_dead() const {
            return this->_state == State::Dead;
        };

        [[nodiscard]] inline bool is_inuse() const {
            return this->_state == State::InUse;
        };

        inline void set_free() {
            this->_state = State::Free;
        };

        inline void set_dead() {
            this->_state = State::Dead;
        };

        inline void set_inuse() {
            this->_state = State::InUse;
        };

        /**
         * 获取状态对应的字符
         * 死亡状态(Dead)   使用 D
         * 空闲状态(Free)   使用 F
         * 使用状态(InUse)  使用 U
         * @return
         */
        [[nodiscard]]  char get_state_char() const;

        /**
         * 增加 内存块等级
         * 即内存块大小缩小两倍
         */
        inline void inc_level() {
            this->_level = (SegmentLevel)((SegementLevel_t)this->_level + 1);
            assert(level_is_valid(this->_level), "segement level is invalid");
        };

        inline void dec_level() {
            this->_level = (SegmentLevel)((SegementLevel_t)this->_level - 1 );
            assert(level_is_valid(this->_level), "segment level is invalid");
        };

        [[nodiscard]] inline SegmentLevel level() const {
            return this->_level;
        };

        /**
         * 判断当前块是不是根块
         * @return
         */
        [[nodiscard]] inline bool is_root_segment() const {
            return this->_level == SegmentLevel::LV_ROOT;
        };

        /**
         * 如果这个segment是它的buddy对中的leader，则返回true，否则返回false。不要调用根块。
         * @return
         */
        [[nodiscard]] bool is_leader() const {
            assert(!this->is_root_segment(), "root segment does not have partner ");
            return is_aligned(
                    (size_t) this->base(),
                    level_to_bytes(this->_level));
        };

        /**
         * 在提交内存的限制下 分配大小
         * @param request_bytes 需求大小 必须对齐
         * @return 无法分配时返回空
         */
        void *allocate(size_t request_bytes);

        /**
         * 确保已提交内存中 未被分配出去内存，满足需求
         * 当不足时 会获取元空间锁  进行新的提交
         * 共
         * @param bytes 需求的内存
         * @return
         */
        bool ensure_committed_enough_and_acquire_lock(size_t bytes);

        /**
         * 调用者在已经获取元空间锁的情况下才可以调用
         * @param bytes
         * @return
         */
        bool ensure_committed_enough(size_t bytes);

        /**
         * 将整个内存块的内存撤销提交
         * 这个必须已经获取元空间锁才可以调用
         */
        void uncommit();

        /**
         * 打印当前节点的信息
         * @param out
         */
        void print_on(CharOStream *out) const;
    };
}

#define SEGMENT_FORMAT               \
    "Segment@" PTR_FORMAT   ","  SEGMENT_LV_FORMAT "%c,base " PTR_FORMAT

#define SEGMENT_FORMAT_ARGS(segment)   \
    segment,(segment)->level(),(segment)->get_state_char(),(segment)->base()

#define SEGMENT_FULL_FORMAT         \
    SEGMENT_FORMAT "(" SIZE_FORMAT " byte),used:" SIZE_FORMAT " byte,committed:" \
    SIZE_FORMAT  " byte,committed-free:" SIZE_FORMAT " byte"

#define SEGMENT_FULL_FORMAT_ARGS(segment)     \
    SEGMENT_FORMAT_ARGS(segment),(segment)->total_bytes(), \
    (segment)->used_bytes(),(segment)->committed_bytes(),     \
    (segment)->free_below_committed_bytes()
#endif //KERNEL_METASPACE_SEGMENT_HPP
