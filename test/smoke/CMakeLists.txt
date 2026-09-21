# Known current blocker (Phase 3, not a pipeline bug): hal_trace.c's crash
# path calls nv_record_flash_flush(), defined in the unconverted
# services/nvrecord/. Everything up to the final link (cross-compile, LDS
# preprocessing, objcopy) is proven working; this one undefined reference
# will resolve itself once services/nvrecord gets a CMakeLists.txt.
opb_add_firmware_image(opb_smoke_test
    LDS ${CMAKE_SOURCE_DIR}/src/scripts/link/best1000.lds.S
    SOURCES
        smoke_main.c
        ${CMAKE_SOURCE_DIR}/src/platform/main/startup_main.S
    LINK_LIBRARIES
        rtos
        platform_hal
        platform_cmsis
        platform_drivers_ana
        platform_drivers_codec
        platform_drivers_norflash
        util_hwtimer_list
        util_boot_struct
)
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/platform/main/startup_main.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
target_include_directories(opb_smoke_test PRIVATE ${CMAKE_SOURCE_DIR}/src/include/rtos/rtx)
