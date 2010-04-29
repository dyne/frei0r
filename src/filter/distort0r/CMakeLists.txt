set (SOURCES distort0r.c)

if (MSVC)
  set_source_files_properties (distort0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (distort0r MODULE ${SOURCES})

install (TARGETS distort0r LIBRARY DESTINATION lib)
