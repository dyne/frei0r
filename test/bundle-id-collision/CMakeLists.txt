cmake_minimum_required(VERSION 3.12)
project(bundle_id_collision C)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/Frei0rBundlePlugin.cmake)
frei0r_bundle_register_id("same-id")
frei0r_bundle_register_id("same_id")
