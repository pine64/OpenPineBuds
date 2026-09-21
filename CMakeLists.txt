cmake_minimum_required (VERSION 3.12)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project (OpenPineBuds C CXX ASM)

option (OPB_BLUE_CONN_LIGHT "Enable the blue connection light" OFF) # Use BLE to control this.

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(compile-flags)
include(target-defaults)
include(options)
include(image)

add_subdirectory (src)

option (OPB_BUILD_SMOKE_TEST "Build a minimal firmware image against the ported subset of the tree, to regression-test the toolchain/link/image pipeline independently of unported apps/services" ON)
if(OPB_BUILD_SMOKE_TEST)
    add_subdirectory (test/smoke)
endif()
