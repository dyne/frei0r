set (SOURCES IIRblur.c fibe.h)
set (TARGET IIRblur)

if (MSVC)
  set_source_files_properties (IIRblur.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

link_libraries(m)
add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
