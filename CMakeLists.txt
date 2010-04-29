cmake_minimum_required (VERSION 2.8)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

project (frei0r)

find_package (OpenCV)

include_directories (AFTER include)

if (MSVC)
  include_directories (include/msvc)
endif (MSVC)

set (FREI0R_DEF ${CMAKE_SOURCE_DIR}/msvc/frei0r_1_0.def)
set (FREI0R_1_1_DEF ${CMAKE_SOURCE_DIR}/msvc/frei0r_1_1.def)

add_subdirectory (src)
