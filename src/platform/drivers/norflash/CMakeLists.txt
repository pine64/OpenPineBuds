file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (platform_drivers_norflash STATIC ${sources} ${headers})
target_link_libraries(platform_drivers_norflash util_boot_struct)
target_include_directories(platform_drivers_norflash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(platform_drivers_norflash PRIVATE
    ${CMAKE_SOURCE_DIR}/services/nvrecord
    ${CMAKE_SOURCE_DIR}/tests/programmer/inc
)