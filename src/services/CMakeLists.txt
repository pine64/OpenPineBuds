add_subdirectory(norflash_api)
add_subdirectory(nv_section)
add_subdirectory(overlay)
add_subdirectory(resources)
add_subdirectory(bt_if_enhanced)
add_subdirectory(bt_profiles_enhanced)
add_subdirectory(nvrecord)
add_subdirectory(ibrt_core)
add_subdirectory(ibrt_ui)
add_subdirectory(cp_accel)
add_subdirectory(osif)
add_subdirectory(audio_process)
add_subdirectory(audioflinger)
add_subdirectory(communication)
add_subdirectory(audio_dump)
add_subdirectory(hw_dsp)
# TODO(Phase 3): bt_app needs the multimedia audio-codec subsystem
# (services/multimedia/audio/codec/{fdkaac,sbc,...}, filters/drc/resample) —
# a large, mostly-unconverted subsystem in its own right. Deferred, along
# with app_ibrt/app_tws which both need bt_app's headers.
#add_subdirectory(bt_app)
#add_subdirectory(app_ibrt)
#add_subdirectory(app_tws)
