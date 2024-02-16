cmake_minimum_required (VERSION 3.12)
project (OpenPineBuds C CXX)

option (OPB_BLUE_CONN_LIGHT "Enable the blue connection light" OFF) # Use BLE to control this.

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory (src)
