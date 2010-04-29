set (SOURCES colordistance.c)

if (MSVC)
  set_source_files_properties (colordistance.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (colordistance MODULE ${SOURCES})

install (TARGETS colordistance LIBRARY DESTINATION lib)
