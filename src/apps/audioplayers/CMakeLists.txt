add_subdirectory (a2dp_decoder)
add_subdirectory (rbplay)

file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_audioplayers STATIC ${sources} ${headers})
