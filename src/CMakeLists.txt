function(generate_dynamic_lib path )
    string(REGEX REPLACE "[/\\]" "-" DYNC_NAME "${path}")
    file(GLOB_RECURSE CPP_LIST ${path}/*.cpp ${path}/*.c)
    message(STATUS "loading ${path} --> ${DYNC_NAME} ....")
    message("\t${CPP_LIST}")
    message(STATUS "----")
    add_library(${DYNC_NAME} SHARED ${CPP_LIST})
    target_link_libraries(${PROJECT_NAME} INTERFACE ${DYNC_NAME})
endfunction()



# 关于 PLATFORM 模块的配置
include_directories(platform/include)

generate_dynamic_lib(platform)
generate_dynamic_lib(metaspace)