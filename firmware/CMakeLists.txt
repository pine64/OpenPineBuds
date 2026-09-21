# The actual bootable open_source.bin (src/build.sh: `make T=open_source
# DEBUG=1`), as opposed to test/smoke/'s synthetic regression-test image.
# platform_main provides the real main()/reset-vector pair and pulls in
# apps_main, which transitively pulls in (via plain, PUBLIC-by-default
# target_link_libraries()) almost everything else — the extra entries below
# are only for libraries nothing else references directly (mirroring
# test/smoke/CMakeLists.txt's own such list for the same underlying reason:
# CMake can't link a dependency purely because something needs it at
# runtime through a function pointer/weak-symbol override rather than a
# direct call).
opb_add_firmware_image(open_source
    LDS ${CMAKE_SOURCE_DIR}/src/scripts/link/best1000.lds.S
    SOURCES
    LINK_LIBRARIES
        platform_main
        apps_main
        config
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
