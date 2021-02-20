set (SOURCES hueshift0r.c matrix.h)
set (TARGET hueshift0r)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

if(NOT MSVC)
  link_libraries(m)
endif()
add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
