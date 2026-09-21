file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_nv_section_factory STATIC ${sources} ${headers})
target_link_libraries(services_nv_section_factory platform_hal platform_drivers_ana util_heap util_crc32 services_norflash_api)
target_include_directories(services_nv_section_factory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
