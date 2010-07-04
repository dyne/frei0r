set (SOURCES composition.c)

if (MSVC)
  set_source_files_properties (composition.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (composition MODULE ${SOURCES})

install (TARGETS composition LIBRARY DESTINATION lib/frei0r-1)
