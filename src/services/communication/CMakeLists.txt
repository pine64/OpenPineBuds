# services/communication/Makefile: comminication_knowles/ only added when
# USE_KNOWLES=1 (open_source/target.mk default: 0) — not included.
file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_communication STATIC ${sources} ${headers})
target_include_directories(services_communication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_communication platform_drivers_ana util_crc32 services_bt_if_enhanced)
target_include_directories(services_communication PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/common
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/comminication_knowles # header-only: USE_KNOWLES=0
)
