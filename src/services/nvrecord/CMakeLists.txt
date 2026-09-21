# services/nvrecord/Makefile
file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_nvrecord STATIC ${sources} ${headers})
target_include_directories(services_nvrecord PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_nvrecord
    util_crc32
    util_heap
    util_encrypt
    util_hexdump
    platform_drivers_ana
    services_norflash_api
    services_bt_if_enhanced
    services_nv_section_customparam
)
target_include_directories(services_nvrecord PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/ble_stack/common/api
    ${CMAKE_SOURCE_DIR}/src/services/ble_stack/ble_ip
)
