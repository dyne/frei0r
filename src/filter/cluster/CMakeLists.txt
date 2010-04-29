set (SOURCES cluster.c)

if (MSVC)
  set_source_files_properties (cluster.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (cluster MODULE ${SOURCES})

install (TARGETS cluster LIBRARY DESTINATION lib)
