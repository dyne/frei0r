set (SOURCES 3dflippo.c)
set (TARGET 3dflippo)

if (MSVC)
  set_source_files_properties (3dflippo.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
