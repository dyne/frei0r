set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
if(NOT MSVC)
  link_libraries(m)
endif()
add_subdirectory (filter)
add_subdirectory (generator)
add_subdirectory (mixer2)
add_subdirectory (mixer3)
