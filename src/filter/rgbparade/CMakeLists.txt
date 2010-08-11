if (MSVC)
  set_source_files_properties (rgbparade.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (${TARGET}  MODULE rgbparade.c rgbparade_image.h)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
