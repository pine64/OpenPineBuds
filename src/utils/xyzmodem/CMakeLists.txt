file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_xyzmodem STATIC ${sources} ${headers})
target_link_libraries(util_xyzmodem platform_hal)
target_include_directories(util_xyzmodem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})