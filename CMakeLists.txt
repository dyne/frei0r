cmake_minimum_required (VERSION 3.12...3.31)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

project (frei0r)
set (VERSION 1.8)

include(GNUInstallDirs)

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

include_directories (AFTER include)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set (LIBDIR "${CMAKE_INSTALL_LIBDIR}/frei0r-1")
set (FREI0R_DEF "${CMAKE_CURRENT_SOURCE_DIR}/msvc/frei0r_1_0.def")
set (FREI0R_1_1_DEF "${CMAKE_CURRENT_SOURCE_DIR}/msvc/frei0r_1_1.def")

# --- custom targets: ---
INCLUDE( cmake/modules/TargetDistclean.cmake OPTIONAL)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# For code documentation run: doxygen doc/Doxyfile
# add_subdirectory (doc)

add_subdirectory (src)

# Generate frei0r.pc and install it.
set (prefix "${CMAKE_INSTALL_PREFIX}")
set (exec_prefix "${CMAKE_INSTALL_PREFIX}")
set (libdir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set (includedir "${CMAKE_INSTALL_PREFIX}/include")
configure_file ("frei0r.pc.in" "frei0r.pc" @ONLY)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/frei0r.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
