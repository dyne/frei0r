cmake_minimum_required(VERSION 3.12)
project(bundle_duplicate_registry C)

set(FREI0R_BUNDLE_REGISTRY_DECLARATIONS
    "extern const f0r_plugin_descriptor_t duplicate_one;\nextern const f0r_plugin_descriptor_t duplicate_two;")
set(FREI0R_BUNDLE_REGISTRY_ENTRIES "  &duplicate_one,\n  &duplicate_two,\n")
configure_file(
  "${FREI0R_SOURCE_DIR}/cmake/Frei0rBundleRegistry.c.in"
  "${CMAKE_CURRENT_BINARY_DIR}/bundle-registry.c" @ONLY
)
add_executable(bundle-duplicate-registry
  "${CMAKE_CURRENT_BINARY_DIR}/bundle-registry.c"
  duplicate-registry-contract.c
)
target_include_directories(bundle-duplicate-registry PRIVATE
  "${FREI0R_SOURCE_DIR}/include"
)
