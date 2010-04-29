set (SOURCES lenscorrection.c)

if (MSVC)
  set_source_files_properties (lenscorrection.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (lenscorrection MODULE ${SOURCES})

install (TARGETS lenscorrection LIBRARY DESTINATION lib)
