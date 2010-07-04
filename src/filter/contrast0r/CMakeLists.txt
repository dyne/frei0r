set (SOURCES contrast0r.c)

if (MSVC)
  set_source_files_properties (contrast0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (contrast0r MODULE ${SOURCES})

install (TARGETS contrast0r LIBRARY DESTINATION lib/frei0r-1)
