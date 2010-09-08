set (SOURCES rgbparade.c rgbparade_image.h)
set (TARGET rgbparade)

if (MSVC)
  set_source_files_properties (rgbparade.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

include_directories (${GAVL_INCLUDE_DIRS})
LINK_LIBRARIES(${GAVL_LIBRARIES})

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
