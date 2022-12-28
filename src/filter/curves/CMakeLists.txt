set (SOURCES curves.c)
set (TARGET curves)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "" C_STANDARD 99)

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
