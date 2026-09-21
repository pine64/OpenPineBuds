# thirdparty/audio_codec_lib/ldac/Makefile: `obj-y += ldac_lib.a` with no
# local source (dec_src/ only — this Makefile only builds the *decoder*
# path unconditionally; the encoder used for A2DP source-side LDAC is a
# closed-source prebuilt archive). Sony's LDAC codec: closed-source,
# matching the BT-stack/multimedia pattern elsewhere in this port.
add_library (thirdparty_ldac STATIC IMPORTED GLOBAL)
set_target_properties(thirdparty_ldac PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/ldac_lib.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
