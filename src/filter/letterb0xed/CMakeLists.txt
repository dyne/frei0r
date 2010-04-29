set (SOURCES letterb0xed.c)

if (MSVC)
  set_source_files_properties (letterb0xed.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (letterb0xed MODULE ${SOURCES})

install (TARGETS letterb0xed LIBRARY DESTINATION lib)
