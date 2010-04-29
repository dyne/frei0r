set (SOURCES uvmap.c)

if (MSVC)
  set_source_files_properties (uvmap.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (uvmap MODULE ${SOURCES})

install (TARGETS uvmap LIBRARY DESTINATION lib)
