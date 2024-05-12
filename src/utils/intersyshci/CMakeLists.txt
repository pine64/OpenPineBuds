file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_intersyshci STATIC ${sources} ${headers})
target_link_libraries(util_intersyshci apps_common util_cqueue platform_drivers_bt services_bt_app)
target_link_libraries(util_intersyshci ${CMAKE_CURRENT_SOURCE_DIR}/lib/libintersyshci_enhanced_stack_RTX.a)
target_include_directories(util_hwtimer_list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
