cmake_minimum_required(VERSION 3.16)
project(frei0r_whirlpinch0r LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_path(FREI0R_INCLUDE_DIR frei0r.h REQUIRED)

add_library(whirlpinch0r MODULE whirlpinch0r.c)
target_include_directories(whirlpinch0r PRIVATE "${FREI0R_INCLUDE_DIR}")
target_compile_options(whirlpinch0r PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -ffast-math -fno-math-errno>
)
set_target_properties(whirlpinch0r PROPERTIES
    PREFIX ""
    OUTPUT_NAME "whirlpinch0r"
)
