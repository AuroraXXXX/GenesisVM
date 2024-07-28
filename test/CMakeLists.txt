message("OPEN TEST ...")
add_executable(test kernel/test_thread.cpp)
target_link_libraries( test   ${PROJECT_NAME})
# 添加测试用例
add_test(NAME test COMMAND test)