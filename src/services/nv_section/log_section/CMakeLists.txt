file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_nv_section_log STATIC ${sources} ${headers})
target_link_libraries(services_nv_section_log util_crc32 util_heap platform_drivers_ana services_norflash_api)
target_include_directories(services_nv_section_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# services/nv_section/log_section/Makefile (TRACE_DUMP2FLASH/TRACE_CRLF both
# unset/0 for open_source — no extra defines needed)
