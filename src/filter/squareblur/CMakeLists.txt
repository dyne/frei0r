set (SOURCES squareblur.c)

if (MSVC)
  set_source_files_properties (squareblur.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (squareblur MODULE ${SOURCES})

install (TARGETS squareblur LIBRARY DESTINATION lib)
