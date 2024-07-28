//
// Created by aurora on 2023/9/27.
//

#ifndef KERNEL_NATIVE_CALL_STACK_HPP
#define KERNEL_NATIVE_CALL_STACK_HPP

#include "stdtype.hpp"
#include "plat/macro.hpp"

#define CALLER_STACK (NativeCallStack(0))

/**
 * 打印
 * 内部C语言栈桢
 */
class NativeCallStack {
public:
    constexpr inline static uint16_t MAX_DEPTH = 5;
private:
    /**
     * 空的堆栈
     */
    static NativeCallStack _empty_stack;
    /**
     * 存储堆栈的指针
     * 0 ~ DEPTH -1
     * 堆栈底部 ～ 堆栈顶部
     *
     */
    void *_stack[MAX_DEPTH];
    NONCOPYABLE(NativeCallStack);

public:
    explicit NativeCallStack() noexcept;

    /**
     * 获取调用堆栈信息
     * @param depth
     */
    explicit NativeCallStack(uint16_t depth);

    /**
     * 拷贝数据
     * @param pc
     * @param count
     */
    explicit NativeCallStack(void* *pc, uint16_t count);

    /**
     * 获取空的堆栈记录
     * @return
     */
    inline static NativeCallStack &empty_stack() { return _empty_stack; };

    /**
     * 记录调用者指针
     */
    void record_current_caller();
    void copy_from(const NativeCallStack& call_stack);
    /**
     * 返回记录的栈桢个数
     * @return
     */
    int frames();

    [[nodiscard]] void* top() const;

    /**
     * 返回栈底指针
     * @return
     */
    [[nodiscard]] inline auto *stack()const {
        return this->_stack;
    };

    [[nodiscard]] inline bool is_empty() const{
        return this->_stack[0] == nullptr;
    };
};


#endif //KERNEL_NATIVE_CALL_STACK_HPP
