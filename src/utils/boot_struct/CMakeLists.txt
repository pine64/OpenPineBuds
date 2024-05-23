file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_boot_struct STATIC ${sources} ${headers})
target_link_libraries(util_boot_struct platform_hal)
target_include_directories(util_boot_struct PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})