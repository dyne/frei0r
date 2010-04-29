set (SOURCES bw0r.c)

if (MSVC)
  set_source_files_properties (bw0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (bw0r MODULE ${SOURCES})

install (TARGETS bw0r LIBRARY DESTINATION lib)
