set (SOURCES hueshift0r.c matrix.h)

if (MSVC)
  set_source_files_properties (hueshift0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (hueshift0r MODULE ${SOURCES})

install (TARGETS hueshift0r LIBRARY DESTINATION lib/frei0r-1)
