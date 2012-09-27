set (SOURCES sigmoidaltransfer.c)
set (TARGET sigmoidaltransfer)

if (MSVC)
  set_source_files_properties (sigmoidaltransfer.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
