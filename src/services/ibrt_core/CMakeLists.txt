# services/ibrt_core/Makefile: no local source under src/ for this config —
# closed-source prebuilt archive. Name matches open_source/target.mk:
# ENHANCED_STACK=1, BLE=0, ANC_APP=1, KERNEL=RTX.
add_library (services_ibrt_core STATIC IMPORTED GLOBAL)
set_target_properties(services_ibrt_core PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libtws_ibrt_enhanced_stack_anc_RTX.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
