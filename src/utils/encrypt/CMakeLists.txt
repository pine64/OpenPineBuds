file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_encrypt STATIC ${sources} ${headers})
target_link_libraries(util_encrypt apps_common util_cqueue)
target_include_directories(util_encrypt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util_encrypt PRIVATE
    ${CMAKE_SOURCE_DIR}/services/bt_profiles/inc/
    ${CMAKE_SOURCE_DIR}/services/bt_if/inc/
    ${CMAKE_SOURCE_DIR}/services/bt_app/
)
target_link_libraries(util_encrypt ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcryption.a)