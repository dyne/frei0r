set (SOURCES 3dflippo.c)

if (MSVC)
  set_source_files_properties (3dflippo.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (3dflippo MODULE ${SOURCES})

install (TARGETS 3dflippo LIBRARY DESTINATION lib)
