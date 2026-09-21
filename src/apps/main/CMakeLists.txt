file (GLOB headers "*.h")
file (GLOB sources "*.c*")

# apps/main/Makefile: CFLAGS_apps.o += $(LDS_SECTION_FLAGS) — the same
# section-size macros cmake/image.cmake computes for the linker script
# (OPB_LDS_* there), also needed by the C code that references these
# section boundaries directly.
add_library (apps_main STATIC ${sources} ${headers})
set_property(SOURCE apps.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    HOTWORD_SECTION_SIZE=${OPB_LDS_HOTWORD_SECTION_SIZE}
    CORE_DUMP_SECTION_SIZE=${OPB_LDS_CORE_DUMP_SECTION_SIZE}
    OTA_UPGRADE_LOG_SIZE=${OPB_LDS_OTA_UPGRADE_LOG_SIZE}
    LOG_DUMP_SECTION_SIZE=${OPB_LDS_LOG_DUMP_SECTION_SIZE}
    CRASH_DUMP_SECTION_SIZE=${OPB_LDS_CRASH_DUMP_SECTION_SIZE}
    CUSTOM_PARAMETER_SECTION_SIZE=${OPB_LDS_CUSTOM_PARAMETER_SECTION_SIZE}
    LHDC_LICENSE_SECTION_SIZE=${OPB_LDS_LHDC_LICENSE_SECTION_SIZE}
    USERDATA_SECTION_SIZE=${OPB_LDS_USERDATA_SECTION_SIZE}
    AUD_SECTION_SIZE=${OPB_LDS_AUD_SECTION_SIZE}
    RESERVED_SECTION_SIZE=${OPB_LDS_RESERVED_SECTION_SIZE}
    FACTORY_SECTION_SIZE=${OPB_LDS_FACTORY_SECTION_SIZE}
)
target_include_directories(apps_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apps_main platform_hal apps_key apps_common services_bt_if_enhanced
    services_bt_app services_nvrecord services_resources services_overlay services_norflash_api
    services_nv_section_factory services_nv_section_log services_ibrt_core services_ibrt_ui
    services_app_ibrt services_app_tws apps_pwl apps_audioplayers)
target_include_directories(apps_main PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/apptester
    ${CMAKE_SOURCE_DIR}/src/utils/cqueue
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/audioflinger
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/codec/sbc/inc
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/apps/battery
    ${CMAKE_SOURCE_DIR}/src/services/ble_stack/hl/api
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_core/inc
    ${CMAKE_SOURCE_DIR}/src/services/ibrt_ui/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_ibrt/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_tws/inc
    ${CMAKE_SOURCE_DIR}/src/services/norflash_api
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/log_section
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/factory_section
    ${CMAKE_SOURCE_DIR}/src/services/nvrecord
    ${CMAKE_SOURCE_DIR}/src/services/resources
    ${CMAKE_SOURCE_DIR}/src/services/overlay
    ${CMAKE_SOURCE_DIR}/src/apps/pwl
    ${CMAKE_SOURCE_DIR}/src/services/ble_app/app_datapath
)