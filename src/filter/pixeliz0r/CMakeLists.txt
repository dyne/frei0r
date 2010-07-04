set (SOURCES pixeliz0r.c)

if (MSVC)
  set_source_files_properties (pixeliz0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (pixeliz0r MODULE ${SOURCES})

install (TARGETS pixeliz0r LIBRARY DESTINATION lib/frei0r-1)
