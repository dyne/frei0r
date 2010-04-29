set (SOURCES tehRoxx0r.c)

if (MSVC)
  set_source_files_properties (tehRoxx0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (tehRoxx0r MODULE ${SOURCES})

install (TARGETS tehRoxx0r LIBRARY DESTINATION lib)
