file (GLOB sources "*.c*" "*/*.c*")

add_library (services_bt_app_a2dp_codecs STATIC ${sources})
target_include_directories(services_bt_app_a2dp_codecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(services_bt_app_a2dp_codecs
    platform_hal
    platform_drivers_ana
    platform_drivers_bt
    util_cqueue
    util_heap
    util_intersyshci
    services_multimedia
    services_audio_process
    services_hw_dsp
    services_overlay
    services_nvrecord
    services_resources
    services_bt_if_enhanced
    thirdparty_ldac # A2DP_LDAC_ON=1 (open_source/target.mk)
)
target_include_directories(services_bt_app_a2dp_codecs PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat
    ${CMAKE_SOURCE_DIR}/src/services/fs/sd
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat/ChaN
    ${CMAKE_SOURCE_DIR}/src/services/audioflinger
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/common
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers/a2dp_decoder
    ${CMAKE_SOURCE_DIR}/src/apps/battery
    ${CMAKE_SOURCE_DIR}/src/apps/factory
    ${CMAKE_SOURCE_DIR}/src/apps/anc/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_ibrt/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_ui/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_core/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/utils/lockcqueue # header-only: not in core-y for open_source
)
