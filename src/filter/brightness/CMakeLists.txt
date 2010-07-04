set (SOURCES brightness.c)

if (MSVC)
  set_source_files_properties (brightness.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (brightness MODULE ${SOURCES})

install (TARGETS brightness LIBRARY DESTINATION lib/frei0r-1)
