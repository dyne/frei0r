set (SOURCES ising0r.c)

if (MSVC)
  set_source_files_properties (ising0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (ising0r MODULE ${SOURCES})

install (TARGETS ising0r LIBRARY DESTINATION lib)
