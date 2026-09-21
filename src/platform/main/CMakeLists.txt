# platform/main/Makefile: obj-y := startup_main.S, then main.cpp (NOSTD=0,
# NOAPP=0, FACTORY_SUITE=0, RELIABILITY_TEST=0 for open_source) — the real
# firmware entry point, as opposed to test/smoke's synthetic main() (which
# reuses only startup_main.S, the reset vector).
add_library (platform_main STATIC main.cpp startup_main.S)
set_property(SOURCE startup_main.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

target_include_directories(platform_main PRIVATE
    ${CMAKE_SOURCE_DIR}/src/utils/hwtimer_list
    ${CMAKE_SOURCE_DIR}/src/platform/drivers/ana
    ${CMAKE_SOURCE_DIR}/src/apps/main
    ${CMAKE_SOURCE_DIR}/src/apps/factory
    ${CMAKE_SOURCE_DIR}/src/utils/list
    ${CMAKE_SOURCE_DIR}/src/apps/audioplayers
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/apps/common
    ${CMAKE_SOURCE_DIR}/src/services/norflash_api
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/log_section
    ${CMAKE_SOURCE_DIR}/src/rtos/rtx/TARGET_CORTEX_M
)
# platform/main/Makefile: ifneq ($(FLASH_SIZE),) ccflags-y += -DFLASH_SIZE=$(FLASH_SIZE)
target_compile_definitions(platform_main PRIVATE FLASH_SIZE=${FLASH_SIZE})
target_link_libraries(platform_main
    platform_hal
    util_hwtimer_list
    apps_common
    apps_main
    services_bt_app
    services_norflash_api
)
