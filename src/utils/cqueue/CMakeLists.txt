file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_cqueue STATIC ${sources} ${headers})
target_link_libraries(util_cqueue platform_cmsis)
target_include_directories(util_cqueue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
