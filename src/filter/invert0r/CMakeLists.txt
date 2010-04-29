set (SOURCES invert0r.c)

if (MSVC)
  set_source_files_properties (invert0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (invert0r MODULE ${SOURCES})

install (TARGETS invert0r LIBRARY DESTINATION lib)
