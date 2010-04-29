set (SOURCES perspective.c)

if (MSVC)
  set_source_files_properties (perspective.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (perspective MODULE ${SOURCES})

install (TARGETS perspective LIBRARY DESTINATION lib)
