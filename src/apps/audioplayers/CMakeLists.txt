# apps/audioplayers/Makefile: several files are conditionally excluded.
# open_source/target.mk-derived values: A2DP_DECODER_VER=2 (exclude
# a2dpplay.cpp, add a2dp_decoder/), SPEECH_TX_THIRDPARTY(_ALANGO)=0 (only
# bt_sco_chain.c, not the thirdparty variants), CHIP_HAS_SCO_DMA_SNAPSHOT=1
# for best2300p -> SCO_DMA_SNAPSHOT=1 (exclude voicebtpcmplay.cpp, keep the
# _sco_dma_snapshot variant), RB_CODEC=0 (rbplay/ not built at all).
add_subdirectory (a2dp_decoder)

file (GLOB headers "*.h")
file (GLOB sources "*.c*")
list(REMOVE_ITEM sources
    ${CMAKE_CURRENT_SOURCE_DIR}/a2dpplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bt_sco_chain_thirdparty.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bt_sco_chain_thirdparty_alango.c
    ${CMAKE_CURRENT_SOURCE_DIR}/voicebtpcmplay.cpp
)

# Makefile: ifeq ($(AUDIO_QUEUE_SUPPORT),1) CFLAGS_app_audio.o += -D__AUDIO_QUEUE_SUPPORT__
# (common.mk: AUDIO_QUEUE_SUPPORT ?= 1)
set_property(SOURCE app_audio.cpp APPEND PROPERTY COMPILE_DEFINITIONS __AUDIO_QUEUE_SUPPORT__)

add_library (apps_audioplayers STATIC ${sources} ${headers})
target_include_directories(apps_audioplayers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apps_audioplayers
    util_list
    util_cqueue
    platform_hal
    platform_drivers_ana
    apps_common
    apps_audioplayers_a2dpdecoder
    services_multimedia
    services_audio_process
    services_audio_dump
    services_overlay
    services_norflash_api
    services_nvrecord
    services_osif
    services_cp_accel
    services_audioflinger
    services_resources
)
target_include_directories(apps_audioplayers PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat
    ${CMAKE_SOURCE_DIR}/src/services/fs/sd
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat/ChaN
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/aud_section
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/include
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/log_section
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers/a2dp_decoder
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/bt_app/a2dp_codecs/include
)
