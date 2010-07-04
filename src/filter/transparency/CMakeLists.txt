set (SOURCES transparency.c)

if (MSVC)
  set_source_files_properties (transparency.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (transparency MODULE ${SOURCES})

install (TARGETS transparency LIBRARY DESTINATION lib/frei0r-1)
