# apps/anc/Makefile: anc_assist.c is filtered OUT unless ANC_ASSIST_ENABLED=1
# (0 for open_source — see cmake/options.cmake). It also references
# AUD_INPUT_PATH_AF_ANC/AUD_STREAM_ID_3, which aren't defined anywhere in
# this repository's shipped platform/hal/hal_aud.h (an upstream gap), so it
# wouldn't compile even if included.
set (sources "./src/anc_wnr.c"
             "./src/app_anc.c"
             "./src/peak_detector.c")
if(ANC_ASSIST_ENABLED)
    list(APPEND sources "./src/anc_assist.c")
endif()

set (headers "./inc/anc_wnr.h"
             "./inc/app_anc.h"
             "./inc/anc_assist.h"
             "./inc/peak_detector.h")

add_library (apps_anc STATIC ${sources} ${headers})
target_include_directories(apps_anc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries (apps_anc PUBLIC apps_common platform_hal platform_drivers_ana services_multimedia services_audio_process
    services_audio_dump services_audioflinger services_app_ibrt services_multimedia_anc_cfg)
target_include_directories(apps_anc PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/anc/include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/filters/include
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/aud_section
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/include
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/utils/hwtimer_list
)
