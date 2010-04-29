set (SOURCES c0rners.c interp.h)

if (MSVC)
  set_source_files_properties (c0rners.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (c0rners MODULE ${SOURCES})

install (TARGETS c0rners LIBRARY DESTINATION lib)
