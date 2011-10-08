cmake_minimum_required (VERSION 2.8)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

project (frei0r)

find_package (OpenCV)
include(FindPkgConfig)
if (PKG_CONFIG_FOUND)
   pkg_check_modules(GAVL gavl)
endif (PKG_CONFIG_FOUND)

include_directories (AFTER include)

if (MSVC)
  include_directories (include/msvc)
endif (MSVC)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set (LIBDIR lib/frei0r-1)
set (FREI0R_DEF ${CMAKE_SOURCE_DIR}/msvc/frei0r_1_0.def)
set (FREI0R_1_1_DEF ${CMAKE_SOURCE_DIR}/msvc/frei0r_1_1.def)

# --- custom targets: ---
INCLUDE( cmake/modules/TargetDistclean.cmake OPTIONAL)

# See this thread for a ridiculous discussion about the simple question how to install a header file with CMake: http://www.cmake.org/pipermail/cmake/2009-October/032874.html
install (DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "frei0r.h" PATTERN "msvc" EXCLUDE)


add_subdirectory (src)
