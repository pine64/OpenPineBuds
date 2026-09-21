file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_nv_section_customparam STATIC ${sources} ${headers})
target_link_libraries(services_nv_section_customparam util_crc32 platform_cmsis)
target_include_directories(services_nv_section_customparam PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
