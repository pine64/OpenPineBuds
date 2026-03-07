file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (platform_hal STATIC ${sources} ${headers})
target_link_libraries(platform_hal platform_drivers_ana platform_cmsis util_crash_catcher platform_drivers_norflash util_boot_struct)
target_include_directories(platform_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(platform_hal PRIVATE
    ${CMAKE_SOURCE_DIR}/platform/drivers/spi_norflash/ 
    ${CMAKE_SOURCE_DIR}/tests/programmer/inc/
)
