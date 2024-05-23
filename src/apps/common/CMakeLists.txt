file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_common STATIC ${sources} ${headers})
target_link_libraries(apps_common platform_hal platform_drivers_ana)
target_include_directories(apps_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(apps_common PRIVATE
    ${CMAKE_SOURCE_DIR}/services/ibrt_ui/inc/
    ${CMAKE_SOURCE_DIR}/services/ibrt_core/inc/
    ${CMAKE_SOURCE_DIR}/services/bt_app/
    ${CMAKE_SOURCE_DIR}/services/app_ai/inc/
)