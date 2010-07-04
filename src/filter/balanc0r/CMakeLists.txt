set (SOURCES balanc0r.c)

if (MSVC)
  set_source_files_properties (balanc0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (balanc0r MODULE ${SOURCES})

install (TARGETS balanc0r LIBRARY DESTINATION lib/frei0r-1)
