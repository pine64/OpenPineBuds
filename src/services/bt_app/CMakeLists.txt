# services/bt_app/Makefile: obj_snd := $(wildcard
# ../../config/_default_cfg_src_/res/$(AUDIO)/*), each entry going through
# a `%.snd.cpp: %` pattern rule (src/scripts/build.mk) that shells out to
# src/convert.sh -C, which ffmpeg-transcodes the resource (opus, etc.) to
# 16kHz mono SBC and xxd-dumps it as a `uint8_t <BASENAME>[]` +
# `unsigned int <BASENAME>_len` pair — exactly the symbols
# res_audio_data.h declares extern. Needs ffmpeg + xxd on PATH.
file(GLOB _opb_res_audio_files "${CMAKE_SOURCE_DIR}/src/config/_default_cfg_src_/res/${OPB_AUDIO_LANGUAGE}/*")
set(_opb_res_audio_dir ${CMAKE_CURRENT_BINARY_DIR}/res_audio)
file(MAKE_DIRECTORY ${_opb_res_audio_dir})
set(_opb_res_audio_sources "")
foreach(_res_file ${_opb_res_audio_files})
    get_filename_component(_res_name ${_res_file} NAME)
    set(_snd_cpp ${_opb_res_audio_dir}/${_res_name}.snd.cpp)
    add_custom_command(
        OUTPUT ${_snd_cpp}
        COMMAND sh ${CMAKE_SOURCE_DIR}/src/convert.sh -C ${_res_file} ${_snd_cpp}
        WORKING_DIRECTORY ${_opb_res_audio_dir}
        DEPENDS ${_res_file} ${CMAKE_SOURCE_DIR}/src/convert.sh
        COMMENT "SND     ${_res_name}"
        VERBATIM
    )
    list(APPEND _opb_res_audio_sources ${_snd_cpp})
endforeach()

file (GLOB sources "*.c*")
list(APPEND sources ${_opb_res_audio_sources})
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
