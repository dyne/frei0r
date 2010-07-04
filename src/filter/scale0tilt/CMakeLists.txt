if (MSVC)
  set_source_files_properties (scale0tilt.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (scale0tilt MODULE scale0tilt.c)

install (TARGETS scale0tilt LIBRARY DESTINATION lib/frei0r-1)
