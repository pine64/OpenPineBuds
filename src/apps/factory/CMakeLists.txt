file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_factory STATIC ${sources} ${headers})
target_include_directories(apps_factory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apps_factory
    platform_drivers_bt
    platform_drivers_ana
    util_boot_struct
    util_intersyshci
    util_hwtimer_list
    util_cqueue
    services_resources
    services_overlay
    services_nvrecord
    services_nv_section_factory
    services_bt_if_enhanced
    util_list
)
target_include_directories(apps_factory PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/algorithm/fft/include
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/battery
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/apps/pwl
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/codec/sbc/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
)
# open_source/target.mk: AUDIO_RESAMPLE ?= 0 -> no __AUDIO_RESAMPLE__ define
