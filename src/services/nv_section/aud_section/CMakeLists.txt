file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_nv_section_aud STATIC ${sources} ${headers})
target_link_libraries(services_nv_section_aud util_crc32)
target_include_directories(services_nv_section_aud PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
