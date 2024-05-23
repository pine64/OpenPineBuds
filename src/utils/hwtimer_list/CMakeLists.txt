file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_hwtimer_list STATIC ${sources} ${headers})
target_link_libraries(util_hwtimer_list platform_hal platform_cmsis)
target_include_directories(util_hwtimer_list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
