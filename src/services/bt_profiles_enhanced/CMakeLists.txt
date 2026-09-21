# See services/bt_if_enhanced/CMakeLists.txt — same situation, no local
# source, prebuilt closed-source archive matching open_source/target.mk's
# config (IBRT=1, HFP_1_6_ENABLE=1, ROM_UTILS_ON=0, BLE=0, BT_RF_PREFER=2M,
# KERNEL=RTX).
add_library (services_bt_profiles_enhanced STATIC IMPORTED GLOBAL)
set_target_properties(services_bt_profiles_enhanced PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/ibrt_libbt_profiles_sbc_enc_2m_RTX.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
