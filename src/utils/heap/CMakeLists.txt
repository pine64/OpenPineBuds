file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_heap STATIC ${sources} ${headers})
target_link_libraries(util_heap platform_cmsis)
target_include_directories(util_heap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
