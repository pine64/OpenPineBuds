# The actual bootable open_source.bin (src/build.sh: `make T=open_source
# DEBUG=1`), as opposed to test/smoke/'s synthetic regression-test image.
#
# This is the complete, already-flattened set of libraries the real link
# needs -- not just platform_main/apps_main plus a few extras. See
# cmake/image.cmake's opb_add_firmware_image() comment for why: the
# -Wl,--start-group/--end-group wrapping it applies only works if nothing
# is left for CMake to auto-append after the group, so every library
# (including ones already reachable transitively through apps_main) is
# spelled out here explicitly. Derived by inspecting the actual failing
# link command and copying every archive CMake was linking.
opb_add_firmware_image(open_source
    LDS ${CMAKE_SOURCE_DIR}/src/scripts/link/best1000.lds.S
    SOURCES
    LINK_LIBRARIES
        platform_main
        apps_main
        apps_anc
        apps_audioplayers
        apps_audioplayers_a2dpdecoder
        apps_battery
        apps_common
        apps_key
        apps_pwl
        config
        platform_cmsis
        platform_drivers_ana
        platform_drivers_bt
        platform_drivers_codec
        platform_drivers_norflash
        platform_hal
        rtos
        services_app_ibrt
        services_app_tws
        services_audio_dump
        services_audio_process
        services_audioflinger
        services_bt_app
        services_bt_app_a2dp_codecs
        services_bt_if_enhanced
        services_bt_profiles_enhanced
        services_communication
        services_cp_accel
        services_hw_dsp
        services_ibrt_core
        services_ibrt_ui
        services_multimedia
        services_norflash_api
        services_nvrecord
        services_nv_section_aud
        services_nv_section_customparam
        services_nv_section_factory
        services_nv_section_log
        services_nv_section_userdata
        services_osif
        services_overlay
        services_resources
        thirdparty_ldac
        util_boot_struct
        util_cqueue
        util_crash_catcher
        util_crc32
        util_encrypt
        util_heap
        util_hexdump
        util_hwtimer_list
        util_intersyshci
        util_list
        util_xyzmodem
)
