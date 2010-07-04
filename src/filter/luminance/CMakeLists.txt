set (SOURCES luminance.c)

if (MSVC)
  set_source_files_properties (luminance.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (luminance MODULE ${SOURCES})

install (TARGETS luminance LIBRARY DESTINATION lib/frei0r-1)
