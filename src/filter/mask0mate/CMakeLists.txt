set (SOURCES mask0mate.c blur.h)

if (MSVC)
  set_source_files_properties (mask0mate.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (mask0mate MODULE ${SOURCES})

install (TARGETS mask0mate LIBRARY DESTINATION lib/frei0r-1)
