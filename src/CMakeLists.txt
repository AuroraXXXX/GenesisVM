function(generate_dynamic_lib path )
    string(REGEX REPLACE "[/\\]" "-" DYNC_NAME "${path}")
    file(GLOB_RECURSE CPP_LIST ${path}/*.cpp ${path}/*.c)
    message(STATUS "loading ${path} --> ${DYNC_NAME} ....")
    message("\t${CPP_LIST}")
    message(STATUS "----")
    add_library(${DYNC_NAME} SHARED ${CPP_LIST})
    target_link_libraries(${PROJECT_NAME} INTERFACE ${DYNC_NAME})
endfunction()




include_directories(plat/include)
include_directories(kernel/include)

generate_dynamic_lib(plat)
generate_dynamic_lib(kernel)
generate_dynamic_lib(global)
generate_dynamic_lib(metaspace)