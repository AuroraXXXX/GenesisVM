cmake_minimum_required(VERSION 3.21)
# 项目名称 起源
set(PROJECT_NAME GenesisVM)
# 项目设置
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_link_options("-g")
add_compile_options(-Werror)
# 调试的时候添加此参数 让地址不随机 编译调试
add_link_options("-no-pie")

include_directories(include)

# 编译最终的产物
option(TEST "是否开启测试" ON)
if (${TEST})
    message('TEST OPEN')
    add_library(${PROJECT_NAME} INTERFACE)
else ()
    message('TEST CLOSED')
    add_executable(${PROJECT_NAME}
            src/posei/utils/robust.cpp)
endif ()
# 添加子模块
add_subdirectory(src)
if (${TEST})
    add_subdirectory(test)
endif ()
