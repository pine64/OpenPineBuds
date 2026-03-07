file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_audioplayers_a2dpdecoder STATIC ${sources} ${headers})
target_link_libraries(apps_audioplayers_a2dpdecoder util_list)
