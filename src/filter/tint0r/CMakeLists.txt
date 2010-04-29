set (SOURCES tint0r.c)

if (MSVC)
  set_source_files_properties (tint0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (tint0r MODULE ${SOURCES})

install (TARGETS tint0r LIBRARY DESTINATION lib)
