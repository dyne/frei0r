cmake_minimum_required(VERSION 3.12)
project(bundle_profile_missing_dependency C)

set(FREI0R_BUILD_BUNDLE ON)
set(FREI0R_BUNDLE_PROFILE core)
set(LIBDIR lib)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/Frei0rBundlePlugin.cmake)

frei0r_add_plugin(
  NAME missing-dependency-plugin KIND FILTER SOURCES missing.c
  BUNDLE_ELIGIBLE BUNDLE_DEPENDENCIES TEST_MISSING_DEPENDENCY)
frei0r_finalize_bundle()
