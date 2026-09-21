file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_osif STATIC ${sources} ${headers})
target_include_directories(services_osif PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_osif util_heap services_bt_if_enhanced)
target_include_directories(services_osif PRIVATE ${CMAKE_SOURCE_DIR}/src/services/nvrecord)
