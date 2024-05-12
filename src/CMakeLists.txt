set(CHIP best2300p)
set(CHIP_BEST2300P 1)
set(CHIP_ID_LITERAL best2300p)

add_compile_definitions(CHIP=${CHIP} CHIP_BEST2300P=${CHIP_BEST2300P} CHIP_ID_LITERAL=${CHIP_ID_LITERAL})

add_subdirectory(apps)
add_subdirectory(utils)
add_subdirectory(platform)
add_subdirectory(rtos)

include_directories (include)
