file (GLOB headers "inc/*.h")
file (GLOB sources "src/*.c*")

add_library (services_app_ibrt STATIC ${sources} ${headers})
target_include_directories(services_app_ibrt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(services_app_ibrt
    platform_hal
    platform_drivers_ana
    platform_drivers_bt
    util_cqueue
    util_heap
    util_intersyshci
    services_osif
    services_overlay
    services_resources
    services_nvrecord
    services_nv_section_factory
    services_bt_if_enhanced
    services_ibrt_core
    services_ibrt_ui
    services_bt_app
    services_multimedia
    apps_anc
)
target_include_directories(services_app_ibrt PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat
    ${CMAKE_SOURCE_DIR}/src/services/fs/sd
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat/ChaN
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/resample/include
    ${CMAKE_SOURCE_DIR}/src/services/audioflinger
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/common
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers/a2dp_decoder
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/factory_section
    ${CMAKE_SOURCE_DIR}/src/apps/anc/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_tws/inc
    ${CMAKE_SOURCE_DIR}/src/apps/battery
    ${CMAKE_SOURCE_DIR}/src/apps/factory
    ${CMAKE_SOURCE_DIR}/src/services/ble_app
    ${CMAKE_SOURCE_DIR}/src/utils/crc16
    ${CMAKE_SOURCE_DIR}/src/utils/lockcqueue # header-only: not in core-y for open_source
)
