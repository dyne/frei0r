set (SOURCES c0rners.c interp.h)
set (TARGET c0rners)

if (MSVC)
  set_source_files_properties (c0rners.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
