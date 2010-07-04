set (SOURCES glow.c blur.h)

if (MSVC)
  set_source_files_properties (glow.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (glow MODULE ${SOURCES})

install (TARGETS glow LIBRARY DESTINATION lib/frei0r-1)
