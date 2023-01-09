set (O_SOURCES alpha0ps_alpha0ps.c fibe_f.h)
set (G_SOURCES alpha0ps_alphagrad.c)
set (S_SOURCES alpha0ps_alphaspot.c)

if (MSVC)
  set (O_SOURCES ${O_SOURCES} ${FREI0R_DEF})
  set (G_SOURCES ${G_SOURCES} ${FREI0R_DEF})
  set (S_SOURCES ${S_SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (alpha0ps_alpha0ps MODULE ${O_SOURCES})
add_library (alpha0ps_alphagrad MODULE ${G_SOURCES})
add_library (alpha0ps_alphaspot MODULE ${S_SOURCES})

if(NOT MSVC)
  target_link_libraries(alpha0ps_alpha0ps -lm)
  target_link_libraries(alpha0ps_alphagrad -lm)
  target_link_libraries(alpha0ps_alphaspot -lm)
endif()

set_target_properties (alpha0ps_alpha0ps PROPERTIES PREFIX "")
set_target_properties (alpha0ps_alphagrad PROPERTIES PREFIX "")
set_target_properties (alpha0ps_alphaspot PROPERTIES PREFIX "")

install (TARGETS alpha0ps_alpha0ps LIBRARY DESTINATION ${LIBDIR})
install (TARGETS alpha0ps_alphagrad LIBRARY DESTINATION ${LIBDIR})
install (TARGETS alpha0ps_alphaspot LIBRARY DESTINATION ${LIBDIR})
