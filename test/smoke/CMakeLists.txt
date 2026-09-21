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
        platform_drivers_bt
        platform_drivers_btpcm
        util_hwtimer_list
        util_boot_struct
        util_libc
        util_sha256
        # hal_trace.c's crash path needs nv_record_flash_flush(); the
        # userdata_section -> nvrecord -> customparam/factory_section chain
        # provides it. platform_hal can't link these directly (it would be
        # circular: they depend on platform_hal), so the final executable
        # target lists them instead — normal for static-archive linking.
        services_nv_section_userdata
        services_nvrecord
        services_osif
        services_overlay
        services_resources
        services_audio_dump
        services_audio_process
        services_audioflinger
        services_communication
        services_cp_accel
        services_hw_dsp
        services_ibrt_core
        services_ibrt_ui
        services_bt_if_enhanced
        services_bt_profiles_enhanced
)
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/platform/main/startup_main.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
target_include_directories(opb_smoke_test PRIVATE ${CMAKE_SOURCE_DIR}/src/include/rtos/rtx)
