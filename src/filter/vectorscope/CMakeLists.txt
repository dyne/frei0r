if (MSVC)
  set_source_files_properties (vectorscope.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (${TARGET}  MODULE vectorscope.c vectorscope_image.h)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
