message("OPEN TEST ...")


function(def_test_case path)
    string(REPLACE "/" "-" RESULT_PATH "${path}")
    add_executable(${RESULT_PATH} ${path}.cpp)
    target_link_libraries(${RESULT_PATH} ${PROJECT_NAME})
    add_test(NAME ${RESULT_PATH} COMMAND ${RESULT_PATH})
    message(STATUS "test case:: ${path}")
endfunction()

def_test_case(kernel/test_thread)