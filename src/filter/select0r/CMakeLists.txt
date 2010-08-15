set (SOURCES select0r.c)

if (MSVC)
  set_source_files_properties (select0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (select0r MODULE ${SOURCES})

install (TARGETS select0r LIBRARY DESTINATION lib)
