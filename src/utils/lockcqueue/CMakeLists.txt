file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_lockcqueue STATIC ${sources} ${headers})
target_link_libraries(util_lockcqueue util_cqueue)
target_include_directories(util_lockcqueue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})