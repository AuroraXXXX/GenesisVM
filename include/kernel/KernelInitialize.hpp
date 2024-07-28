//
// Created by aurora on 2024/7/28.
//

#ifndef GENESISVM_KERNEL_INITIALIZE_HPP
#define GENESISVM_KERNEL_INITIALIZE_HPP
#include "plat/mem/AllStatic.hpp"
class KernelInitialize:public AllStatic{
private:
    static void daemon_thread_initialize();
public:
    static void initialize();
};
#endif //GENESISVM_KERNEL_INITIALIZE_HPP
