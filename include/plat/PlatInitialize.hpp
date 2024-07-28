//
// Created by aurora on 2024/6/29.
//

#ifndef PLATFORM_OS_INIT_HPP
#define PLATFORM_OS_INIT_HPP
#include "stdtype.hpp"
#include "plat/mem/allocation.hpp"
class OSThread;
class PlatInitialize:public AllStatic{
public:
    /**
 * 初始化函数
 * @param vm_start_time vm起始的时间戳
 */
   static void initialize(ticks_t vm_start_time,
                    const char ** tags_name,
                    uint16_t max_tags,
                    OSThread* os_thread);

    static   void destroy();
};
#endif //PLATFORM_OS_INIT_HPP
