cmake_minimum_required (VERSION 2.8)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

project (frei0r)
set (VERSION 1.3.0)

option (WITHOUT_OPENCV "Disable plugins dependent upon OpenCV" OFF)
if (NOT WITHOUT_OPENCV)
  find_package (OpenCV)
endif ()

find_package (Cairo)

include(FindPkgConfig)
option (WITHOUT_GAVL "Disable plugins dependent upon gavl" OFF)
if (PKG_CONFIG_FOUND AND NOT WITHOUT_GAVL)
  pkg_check_modules(GAVL gavl)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_C_COMPILER "clang")
  set(CMAKE_CXX_COMPILER "clang++")
  set(CMAKE_C_FLAGS "-arch x86_64 -arch i386")
  set(CMAKE_CXX_FLAGS "-arch x86_64 -arch i386")
endif ()


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

add_subdirectory (doc)
add_subdirectory (src)

# Generate frei0r.pc and install it.
set (prefix "${CMAKE_INSTALL_PREFIX}")
set (exec_prefix "${CMAKE_INSTALL_PREFIX}")
set (libdir "${CMAKE_INSTALL_PREFIX}/lib")
set (includedir "${CMAKE_INSTALL_PREFIX}/include")
configure_file ("frei0r.pc.in" "frei0r.pc" @ONLY)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/frei0r.pc" DESTINATION lib/pkgconfig)
