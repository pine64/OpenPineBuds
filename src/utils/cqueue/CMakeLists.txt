file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_cqueue STATIC ${sources} ${headers})
target_link_libraries(util_cqueue platform_cmsis)
target_include_directories(util_cqueue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# src/utils/cqueue/Makefile
if(ROM_UTILS_ON)
    target_compile_definitions(util_cqueue PRIVATE CQ_FUNC_ATTR=WEAK)
else()
    target_compile_definitions(util_cqueue PRIVATE CQ_FUNC_ATTR=)
endif()
