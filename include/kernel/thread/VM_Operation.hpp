//
// Created by aurora on 2024/2/27.
//

#ifndef NUCLEUSVM_VM_OPERATION_HPP
#define NUCLEUSVM_VM_OPERATION_HPP

#include "plat/mem/allocation.hpp"


class PlatThread;

/**
 * VM操作函数
 */
class VM_Operation : public StackObject {
    friend class VMThread;

private:
    PlatThread *_calling_thread;

    /**
     * 被VMThread调用，内部会调用doit，不允许进行调用
     */
    void evaluate();

public:
    inline explicit VM_Operation() : _calling_thread(nullptr) {};

    inline void set_calling_thread(PlatThread *thread) {
        this->_calling_thread = thread;
    };

    /**
     * 发起请求的线程
     * @return
     */
    inline auto calling_thread() {
        return this->_calling_thread;
    };

    virtual void doit() = 0;

    /**
     * 返回true 才会调用doit
     * @return
     */
    virtual bool doit_prologue() { return true; };

    /**
     * 调用doit完毕后调用
     */
    virtual void doit_epilogue() {};

    /**
     * 当前 VM_Operation 的名称
     * @return
     */
    virtual const char *name() = 0;

    /**
     * 触发当前 VM_Operation 的原因
     * @return
     */
    [[nodiscard]] virtual const char *cause() const { return nullptr; }

    static void execute(VM_Operation *operation);

    /**
     * 是否允许嵌入式调用
     * @return
     */
    [[nodiscard]] virtual bool allow_nested_vm_operations() const { return false; }

    /**
     * 是否在安全点执行
     * @return
     */
    [[nodiscard]] virtual bool evaluate_at_safepoint() const { return true; }

    virtual void print_on(CharOStream *out);
};


#endif //NUCLEUSVM_VM_OPERATION_HPP
