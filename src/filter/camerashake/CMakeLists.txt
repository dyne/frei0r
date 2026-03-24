cmake_minimum_required(VERSION 3.10)
project(CameraShakePlugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compile as shared library
add_library(camerashake MODULE camerashake.cpp)

# Remove the "lib" prefix so that Frei0r can read it correctly
set_target_properties(camerashake PROPERTIES PREFIX "")
