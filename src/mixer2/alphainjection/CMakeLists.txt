set (SOURCES alphainjection.c)

if (MSVC)
  set_source_files_properties (alphainjection.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (alphainjection MODULE ${SOURCES})

install (TARGETS alphainjection LIBRARY DESTINATION lib/frei0r-1)
