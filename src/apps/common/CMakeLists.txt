file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_common STATIC ${sources} ${headers})
