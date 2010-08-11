if (MSVC)
  set_source_files_properties (scale0tilt.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (${TARGET}  MODULE scale0tilt.c)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
