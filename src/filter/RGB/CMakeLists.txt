set (R_SOURCES R.c)
set (G_SOURCES G.c)
set (B_SOURCES B.c)

if (MSVC)
  set_source_files_properties (R.c G.c B.c PROPERTIES LANGUAGE CXX)
  set (R_SOURCES ${R_SOURCES} ${FREI0R_DEF})
  set (G_SOURCES ${G_SOURCES} ${FREI0R_DEF})
  set (B_SOURCES ${B_SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (R MODULE ${R_SOURCES})
add_library (G MODULE ${G_SOURCES})
add_library (B MODULE ${B_SOURCES})

install (TARGETS R LIBRARY DESTINATION lib/frei0r-1)
install (TARGETS G LIBRARY DESTINATION lib/frei0r-1)
install (TARGETS B LIBRARY DESTINATION lib/frei0r-1)
