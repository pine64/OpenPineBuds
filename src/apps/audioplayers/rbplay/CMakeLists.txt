file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_audioplayers_rbplay STATIC ${sources} ${headers})
