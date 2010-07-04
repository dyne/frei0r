set (SOURCES saturat0r.c)

if (MSVC)
  set_source_files_properties (saturat0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (saturat0r MODULE ${SOURCES})

install (TARGETS saturat0r LIBRARY DESTINATION lib/frei0r-1)
