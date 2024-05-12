file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_audiplayers_a2dpdecoder STATIC ${sources} ${headers})
