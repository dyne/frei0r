# Plugins expose only the frei0r C API; keep implementation symbols private.
set (CMAKE_C_VISIBILITY_PRESET hidden)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_VISIBILITY_INLINES_HIDDEN YES)
add_compile_definitions (FREI0R_PLUGIN)

set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
if(NOT MSVC)
  link_libraries(m)
endif()
add_subdirectory (filter)
add_subdirectory (generator)
add_subdirectory (mixer2)
add_subdirectory (mixer3)
