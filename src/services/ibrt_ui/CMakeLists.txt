# services/ibrt_ui/Makefile: no local source under src/ for this config —
# closed-source prebuilt archive. Name matches open_source/target.mk:
# ENHANCED_STACK=1, BLE=0, KERNEL=RTX (this one doesn't fold in ANC_APP).
add_library (services_ibrt_ui STATIC IMPORTED GLOBAL)
set_target_properties(services_ibrt_ui PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libtws_ibrt_enhanced_stack_RTX.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
