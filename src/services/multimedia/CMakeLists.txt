# services/multimedia/Makefile: every mmlib_obj source directory
# (algorithm/fft/src, audio/process/*/src, audio/codec/sbc/src,
# audio/codec/fdkaac_codec/*/src, speech/src) is absent from this
# repository — only public headers (include/, inc/) ship. Like BES's BT/IBRT
# stack, the DSP/codec library is closed-source: a prebuilt archive per
# build configuration. Name matches open_source/target.mk:
# A2DP_CP_ACCEL=1 -> _cp, ANC_APP=1 -> _anc, ROM_UTILS_ON=0 -> no _romaac.
add_library (services_multimedia STATIC IMPORTED GLOBAL)
set_target_properties(services_multimedia PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/best2300p_libmultimedia_cp_anc.a
)
target_include_directories(services_multimedia INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/speech/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/sbc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/process/filters/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/process/anc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/process/resample/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/process/drc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/process/limiter/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/process/common/include
    # open_source/target.mk: A2DP_AAC_ON=1 (headers only — no fdkaac_codec
    # source ships in this repository, see note above)
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libAACdec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libAACenc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libFDK/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libMpegTPDec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libMpegTPEnc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libPCMutils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libSBRdec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libSBRenc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/codec/fdkaac_codec/libSYS/include
)

# The three cfg/coef directories *do* ship real source (small config-table
# .c files, not algorithm code) and are built normally.
add_subdirectory(audio/process/anc/cfg)
add_subdirectory(audio/process/filters/cfg)
add_subdirectory(audio/process/resample/coef)
