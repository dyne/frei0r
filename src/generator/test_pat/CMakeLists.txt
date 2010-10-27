set (B_SOURCES test_pat_B.c)
set (C_SOURCES test_pat_C.c)
set (G_SOURCES test_pat_G.c)

if (MSVC)
  set_source_files_properties (test_pat_B.c test_pat_C.c test_pat_G.c PROPERTIES LANGUAGE CXX)
  set (B_SOURCES ${B_SOURCES} ${FREI0R_DEF})
  set (C_SOURCES ${C_SOURCES} ${FREI0R_DEF})
  set (G_SOURCES ${G_SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (test_pat_B MODULE ${B_SOURCES})
add_library (test_pat_C MODULE ${C_SOURCES})
add_library (test_pat_G MODULE ${G_SOURCES})

set_target_properties (test_pat_B PROPERTIES PREFIX "")
set_target_properties (test_pat_C PROPERTIES PREFIX "")
set_target_properties (test_pat_G PROPERTIES PREFIX "")

install (TARGETS test_pat_B LIBRARY DESTINATION ${LIBDIR})
install (TARGETS test_pat_C LIBRARY DESTINATION ${LIBDIR})
install (TARGETS test_pat_G LIBRARY DESTINATION ${LIBDIR})
