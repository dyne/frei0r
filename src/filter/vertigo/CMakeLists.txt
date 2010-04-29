set (SOURCES vertigo.c)

if (MSVC)
  set_source_files_properties (vertigo.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (vertigo MODULE ${SOURCES})

install (TARGETS vertigo LIBRARY DESTINATION lib)
