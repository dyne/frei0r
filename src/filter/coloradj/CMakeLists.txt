set (SOURCES coloradj_RGB.c)

if (MSVC)
  set_source_files_properties (coloradj_RGB.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (coloradj_RGB MODULE ${SOURCES})

install (TARGETS coloradj_RGB LIBRARY DESTINATION lib)
