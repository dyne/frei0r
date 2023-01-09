set (B_SOURCES measure_pr0be.c measure.h font2.h)
set (F_SOURCES measure_pr0file.c measure.h font2.h)

if (MSVC)
  set (B_SOURCES ${B_SOURCES} ${FREI0R_DEF})
  set (F_SOURCES ${F_SOURCES} ${FREI0R_DEF})
endif (MSVC)

# link_libraries(m)
add_library (measure_pr0be  MODULE  ${B_SOURCES})
add_library (measure_pr0file  MODULE  ${F_SOURCES})

set_target_properties (measure_pr0be PROPERTIES PREFIX "")
set_target_properties (measure_pr0file PROPERTIES PREFIX "")

install (TARGETS measure_pr0be LIBRARY DESTINATION ${LIBDIR})
install (TARGETS measure_pr0file LIBRARY DESTINATION ${LIBDIR})
