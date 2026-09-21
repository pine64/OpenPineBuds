# bt_if_enhanced/Makefile globs local sources into a dynamically-named
# archive, but no .c/.cpp files exist under this directory — only inc/
# (public headers) and lib/ (prebuilt closed-source archives, one per build
# configuration). This is BES's proprietary BT stack: OpenPineBuds ships the
# headers and a matching prebuilt .a, not source. The name below matches
# open_source/target.mk's config: IBRT=1, HFP_1_6_ENABLE=1, BT_RF_PREFER=2M,
# BLE=0, KERNEL=RTX (see Makefile's BTAPI_LIB_NAME derivation).
add_library (services_bt_if_enhanced STATIC IMPORTED GLOBAL)
set_target_properties(services_bt_if_enhanced PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/ibrt_libbt_api_sbc_enc_2m_RTX.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
