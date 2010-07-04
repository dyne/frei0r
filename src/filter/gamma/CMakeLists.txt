set (SOURCES gamma.c)

if (MSVC)
  set_source_files_properties (gamma.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (gamma MODULE ${SOURCES})

install (TARGETS gamma LIBRARY DESTINATION lib/frei0r-1)
