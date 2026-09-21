# apps/audioplayers/a2dp_decoder/Makefile: this is an EXPLICIT obj-y list,
# not a glob — a2dp_decoder_example.cpp, a2dp_decoder_scalable.cpp (needs
# A2DP_SCALABLE_ON=1), a2dp_decoder_lhdc.o (A2DP_LHDC_ON=1), and
# a2dp_decoder_cp.c are never in obj-y for any flag combination this repo
# documents. open_source/target.mk: A2DP_AAC_ON=1, A2DP_LDAC_ON=1,
# A2DP_SCALABLE_ON=0, A2DP_LHDC_ON=0.
set (sources
    a2dp_decoder.cpp
    a2dp_decoder_sbc.cpp
    a2dp_decoder_aac_lc.cpp
    a2dp_decoder_ldac.cpp
)
file (GLOB headers "*.h")

add_library (apps_audioplayers_a2dpdecoder STATIC ${sources} ${headers})
target_include_directories(apps_audioplayers_a2dpdecoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apps_audioplayers_a2dpdecoder
    util_list
    util_cqueue
    util_heap
    platform_hal
    platform_drivers_ana
    apps_common
    services_bt_if_enhanced
    services_multimedia
    services_audio_process
    services_resources
    services_overlay
    services_norflash_api
    services_nvrecord
    services_audio_dump
    services_audioflinger
    thirdparty_ldac
    platform_drivers_bt
)
target_include_directories(apps_audioplayers_a2dpdecoder PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat
    ${CMAKE_SOURCE_DIR}/src/services/fs/sd
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat/ChaN
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/bt_app/a2dp_codecs/include
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/aud_section
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/include
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/log_section
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/codec/sbc/inc
    ${CMAKE_SOURCE_DIR}/src/utils/cqueue
    ${CMAKE_SOURCE_DIR}/src/platform/drivers/bt
    ${CMAKE_SOURCE_DIR}/src/services/app_ibrt/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_tws/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_core/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_ui/inc
)
