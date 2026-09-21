# services/bt_app/Makefile
# TODO(Phase 3): the Makefile also bakes config/_default_cfg_src_/res/<AUDIO>/
# assets into *.snd.cpp via a generation step (obj_snd) — audio-prompt
# resource embedding isn't wired up yet (needs the AUDIO=<lang> selection
# from PLAN.md's Phase 2 TODOs). Only the app-logic sources are built here.
file (GLOB sources "*.c*")
# app_fp_rfcomm.cpp (Google Fast Pair over classic-BT RFCOMM) references
# RFCOMM_CHANNEL_FP/BTIF_APP_SPP_SERVER_FP_RFCOMM_ID, which are not defined
# anywhere in this repository's shipped bt_if_enhanced/bt_profiles_enhanced
# headers — a pre-existing gap in the open-sourced tree, not a CMake
# conversion issue (GFPS_ENABLE=0 for open_source, so this is dead code
# either way; excluding it rather than guessing values for undocumented
# vendor constants).
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/app_fp_rfcomm.cpp)

add_library (services_bt_app STATIC ${sources})
add_subdirectory(a2dp_codecs)
target_include_directories(services_bt_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_bt_app
    platform_hal
    platform_drivers_ana
    platform_drivers_bt
    util_cqueue
    util_heap
    util_intersyshci
    services_osif
    services_audio_process
    services_audio_dump
    services_hw_dsp
    services_overlay
    services_nvrecord
    services_resources
    services_audioflinger
    services_bt_if_enhanced
    services_bt_profiles_enhanced
    services_bt_app_a2dp_codecs
    services_multimedia
    services_nv_section_aud
    services_nv_section_userdata
)
target_include_directories(services_bt_app PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat
    ${CMAKE_SOURCE_DIR}/src/services/fs/sd
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat/ChaN
    ${CMAKE_SOURCE_DIR}/src/apps/anc/inc
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/mic_alg
    ${CMAKE_SOURCE_DIR}/src/apps/common
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers/a2dp_decoder
    ${CMAKE_SOURCE_DIR}/src/apps/battery
    ${CMAKE_SOURCE_DIR}/src/apps/factory
    ${CMAKE_SOURCE_DIR}/src/services/app_ibrt/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_core/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_ui/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_tws/inc
    ${CMAKE_SOURCE_DIR}/src/utils/hwtimer_list
    ${CMAKE_SOURCE_DIR}/src/utils/intersyshci
    ${CMAKE_SOURCE_DIR}/src/utils/lockcqueue # header-only: not in core-y for open_source
    ${CMAKE_SOURCE_DIR}/src/services/interconnection/umm_malloc
)

target_compile_definitions(services_bt_app PRIVATE
    AUDIO_OUTPUT_VOLUME_DEFAULT=12 # per-file in Makefile, matches the global default
)
set_property(SOURCE besmain.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    BUILD_HOSTNAME=unknown
    BUILD_USERNAME=unknown
)
# open_source/target.mk / common.mk defaults relevant to app_bt_stream.cpp:
set_property(SOURCE app_bt_stream.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    AUDIO_OUTPUT_VOLUME_DEFAULT=12
    RESAMPLE_ANY_SAMPLE_RATE # common.mk always-on
)
if(ANC_APP)
    set_property(SOURCE app_bt_stream.cpp APPEND PROPERTY COMPILE_DEFINITIONS ANC_APP)
    set_property(SOURCE app_media_player.cpp APPEND PROPERTY COMPILE_DEFINITIONS ANC_APP)
endif()
