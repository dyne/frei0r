cmake_minimum_required (VERSION 3.12)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

set (FREI0R_VERSION "3.0.0" CACHE STRING "Project version in SemVer format")
if (DEFINED ENV{FREI0R_VERSION} AND NOT "$ENV{FREI0R_VERSION}" STREQUAL "")
  set (FREI0R_VERSION "$ENV{FREI0R_VERSION}")
endif ()
string (REGEX REPLACE "^v" "" FREI0R_VERSION "${FREI0R_VERSION}")
if (NOT FREI0R_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message (FATAL_ERROR "FREI0R_VERSION must be a SemVer string like 3.0.0")
endif ()

project (frei0r VERSION ${FREI0R_VERSION})
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED True)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CTEST_ARGUMENTS "--verbose")
include(CTest)

option (WITHOUT_OPENCV "Disable plugins dependent upon OpenCV" OFF)
option (WITHOUT_FACERECOGNITION "Disable facedetect plugin to avoid protobuf conflicts" OFF)

if (NOT WITHOUT_OPENCV)
  find_package (OpenCV REQUIRED)
endif ()

option (WITHOUT_CAIRO "Disable plugins dependent upon Cairo" OFF)
if (NOT WITHOUT_CAIRO)
  find_package (Cairo REQUIRED)
endif ()

include(FindPkgConfig)
find_package(Threads REQUIRED)
option (WITHOUT_GAVL "Disable plugins dependent upon gavl" OFF)
if (PKG_CONFIG_FOUND AND NOT WITHOUT_GAVL)
  pkg_check_modules(GAVL REQUIRED gavl)
endif ()

option(FREI0R_BUILD_BUNDLE "Build the experimental static frei0r bundle" OFF)
set(FREI0R_BUNDLE_PROFILE "core" CACHE STRING
    "Bundle profile to resolve when FREI0R_BUNDLE_PLUGINS is empty")
set_property(CACHE FREI0R_BUNDLE_PROFILE PROPERTY STRINGS core all)
set(FREI0R_BUNDLE_PLUGINS "" CACHE STRING
    "Semicolon-separated bundle target names; overrides FREI0R_BUNDLE_PROFILE")

# WebAssembly bundles start from an explicitly scalar, single-threaded profile.
# The native MODULE targets retain their existing platform-specific settings;
# these options affect only the separately compiled bundle objects.
set(_frei0r_wasm_target OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "WASI" OR EMSCRIPTEN)
  set(_frei0r_wasm_target ON)
endif()
option(FREI0R_BUNDLE_WASM_BASELINE
  "Build bundle objects with the portable scalar, single-threaded Wasm baseline"
  ${_frei0r_wasm_target})
option(FREI0R_BUNDLE_WASM_SIMD
  "Build a separate experimental Wasm SIMD bundle profile" OFF)
option(FREI0R_BUNDLE_WASM_PTHREADS
  "Build a separate experimental Wasm pthread bundle profile" OFF)
if((FREI0R_BUNDLE_WASM_BASELINE OR FREI0R_BUNDLE_WASM_SIMD OR
    FREI0R_BUNDLE_WASM_PTHREADS) AND NOT _frei0r_wasm_target)
  message(FATAL_ERROR "FREI0R_BUNDLE_WASM_* options require a WASI or Emscripten target")
endif()
if(FREI0R_BUNDLE_WASM_BASELINE AND
   (FREI0R_BUNDLE_WASM_SIMD OR FREI0R_BUNDLE_WASM_PTHREADS))
  message(FATAL_ERROR "the scalar Wasm baseline cannot be combined with SIMD or pthread profiles")
endif()
if(FREI0R_BUNDLE_WASM_SIMD AND FREI0R_BUNDLE_WASM_PTHREADS)
  message(FATAL_ERROR "Wasm SIMD and pthread profiles must be configured separately")
endif()
if(FREI0R_BUNDLE_WASM_BASELINE)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/Frei0rBundlePlugin.cmake)

include_directories (AFTER include)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set (LIBDIR "${CMAKE_INSTALL_LIBDIR}/frei0r-1")
set (FREI0R_DEF "${CMAKE_SOURCE_DIR}/msvc/frei0r_1_0.def")
set (FREI0R_1_1_DEF "${CMAKE_SOURCE_DIR}/msvc/frei0r_1_1.def")
set (FREI0R_1_2_DEF "${CMAKE_SOURCE_DIR}/msvc/frei0r_1_2.def")

# --- custom targets: ---
INCLUDE( cmake/modules/TargetDistclean.cmake OPTIONAL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# For code documentation run: doxygen doc/Doxyfile
# add_subdirectory (doc)

add_subdirectory (src)
set(FREI0R_BUNDLE_OPENCV_TARGETS facebl0r facedetect)
set(FREI0R_BUNDLE_CAIRO_TARGETS cairoimagegrid cairogradient mirr0r shake0scillate cairoaffineblend cairoblend)
set(FREI0R_BUNDLE_GAVL_TARGETS rgbparade scale0tilt vectorscope)
set(FREI0R_BUNDLE_OPENGL_TARGETS shadert0y)
set(FREI0R_BUNDLE_DYNAMIC_LOADING_TARGETS colgate ndvi)
frei0r_bundle_classify_legacy_modules("${CMAKE_SOURCE_DIR}/src")
frei0r_bundle_promote_core_modules("${CMAKE_SOURCE_DIR}/src")
frei0r_finalize_bundle()
if(FREI0R_BUILD_BUNDLE AND NOT _frei0r_wasm_target)
  set(FREI0R_BUNDLE_CONFIG_DEPENDENCIES)
  if(FREI0R_BUNDLE_NEEDS_THREADS)
    string(APPEND FREI0R_BUNDLE_CONFIG_DEPENDENCIES "find_dependency(Threads)\n")
  endif()
  configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/Frei0rBundleConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Frei0rBundleConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Frei0rBundle"
  )
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/Frei0rBundleConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
  )
  install(EXPORT Frei0rBundleTargets
    FILE Frei0rBundleTargets.cmake
    NAMESPACE Frei0rBundle::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Frei0rBundle"
  )
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Frei0rBundleConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Frei0rBundleConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Frei0rBundle"
  )
endif()
set_property(GLOBAL PROPERTY FREI0R_BUNDLE_CLASSIFICATIONS "")
frei0r_bundle_collect_classifications("${CMAKE_SOURCE_DIR}/src")
get_property(FREI0R_BUNDLE_CLASSIFICATIONS GLOBAL PROPERTY FREI0R_BUNDLE_CLASSIFICATIONS)
set(FREI0R_BUNDLE_CLASSIFICATIONS "${FREI0R_BUNDLE_CLASSIFICATIONS}" CACHE INTERNAL
    "Auditable target/profile/reason classification for bundle selection" FORCE)
if (BUILD_TESTING)
  add_subdirectory (test)
endif ()

# Generate frei0r.pc and install it.
set (prefix "${CMAKE_INSTALL_PREFIX}")
set (exec_prefix "${CMAKE_INSTALL_PREFIX}")
set (libdir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set (includedir "${CMAKE_INSTALL_PREFIX}/include")
configure_file ("frei0r.pc.in" "frei0r.pc" @ONLY)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/frei0r.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
