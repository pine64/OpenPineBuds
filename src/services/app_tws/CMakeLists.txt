file (GLOB headers "inc/*.h")
file (GLOB sources "src/*.c*")

add_library (services_app_tws STATIC ${sources} ${headers})
target_include_directories(services_app_tws PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(services_app_tws
    platform_hal
    platform_drivers_bt
    services_bt_if_enhanced
    services_ibrt_core
    services_ibrt_ui
    services_app_ibrt
    services_bt_app
    services_multimedia
    util_cqueue
)
target_include_directories(services_app_tws PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/key
    ${CMAKE_SOURCE_DIR}/src/services/bt_app
    ${CMAKE_SOURCE_DIR}/src/services/app_ibrt/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_tws/inc
    ${CMAKE_SOURCE_DIR}/src/services/app_ai/inc
    ${CMAKE_SOURCE_DIR}/src/services/ble_app/app_sec
)
