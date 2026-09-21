# services/nv_section/userdata_section/Makefile: *gsound.c filtered out
# unless BISTO_ENABLE=1, *ota.c filtered out unless OTA_ENABLE=1 (both 0 for
# open_source). This is the *active* nvrecord implementation — its sibling
# services/nvrecord/*.c is almost entirely `#if !defined(NEW_NV_RECORD_ENABLED)`,
# a no-op since that macro is defined globally (see cmake/target-defaults.cmake).
set (sources
    nvrecord_ble.c
    nvrecord_bt.c
    nvrecord_dma_config.c
    nvrecord_env.c
    nvrecord_extension.c
    nvrecord_fp_account_key.c
)
file (GLOB headers "*.h")

add_library (services_nv_section_userdata STATIC ${sources} ${headers})
target_include_directories(services_nv_section_userdata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_nv_section_userdata
    platform_hal
    platform_drivers_ana
    platform_drivers_norflash
    util_crc32
    util_heap
    util_hexdump
    util_cqueue
    services_norflash_api
    services_nv_section_customparam
    services_nv_section_factory
    services_bt_if_enhanced
    services_osif
    services_nvrecord
)
target_include_directories(services_nv_section_userdata PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/nv_section
)
