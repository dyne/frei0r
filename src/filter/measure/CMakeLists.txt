set (B_SOURCES pr0be.c measure.h font2.h)
set (F_SOURCES pr0file.c measure.h font2.h)

if (MSVC)
  set_source_files_properties (pr0be.c pr0file.c PROPERTIES LANGUAGE CXX)
  set (B_SOURCES ${B_SOURCES} ${FREI0R_DEF})
  set (F_SOURCES ${F_SOURCES} ${FREI0R_DEF})
endif (MSVC)

link_libraries(m)
add_library (pr0be  MODULE  ${B_SOURCES})
add_library (pr0file  MODULE  ${F_SOURCES})

set_target_properties (pr0be PROPERTIES PREFIX "")
set_target_properties (pr0file PROPERTIES PREFIX "")

install (TARGETS pr0be LIBRARY DESTINATION ${LIBDIR})
install (TARGETS pr0file LIBRARY DESTINATION ${LIBDIR})
