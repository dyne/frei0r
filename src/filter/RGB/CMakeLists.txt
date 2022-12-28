set (R_SOURCES R.c)
set (G_SOURCES G.c)
set (B_SOURCES B.c)

if (MSVC)
  set (R_SOURCES ${R_SOURCES} ${FREI0R_DEF})
  set (G_SOURCES ${G_SOURCES} ${FREI0R_DEF})
  set (B_SOURCES ${B_SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (R MODULE ${R_SOURCES})
add_library (G MODULE ${G_SOURCES})
add_library (B MODULE ${B_SOURCES})

set_target_properties (R PROPERTIES PREFIX "")
set_target_properties (G PROPERTIES PREFIX "")
set_target_properties (B PROPERTIES PREFIX "")

install (TARGETS R LIBRARY DESTINATION ${LIBDIR})
install (TARGETS G LIBRARY DESTINATION ${LIBDIR})
install (TARGETS B LIBRARY DESTINATION ${LIBDIR})
