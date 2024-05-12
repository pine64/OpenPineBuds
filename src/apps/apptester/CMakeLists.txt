file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_apptester STATIC ${sources} ${headers})
target_link_libraries(apps_apptester apps_common)