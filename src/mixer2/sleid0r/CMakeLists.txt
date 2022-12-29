set(MIXERS
  WIPE-UP
  WIPE-LEFT
  WIPE-BARN-DOOR-V
  SLIDE-RIGHT
  PUSH-UP
  PUSH-DOWN
  WIPE-RIGHT
  WIPE-DOWN
  WIPE-BARN-DOOR-H
  SLIDE-LEFT
  PUSH-RIGHT
  WIPE-RECT
  WIPE-CIRCLE
  SLIDE-UP
  SLIDE-DOWN
  PUSH-LEFT
)

foreach(MIXER IN LISTS MIXERS)
  string(TOLOWER ${MIXER} LOWERCASE_MIXER)
  set(${MIXER}_SOURCES sleid0r_${LOWERCASE_MIXER}.c)
  if(MSVC)
	set(${MIXER}_SOURCES ${${MIXER}_SOURCES} ${FREI0R_DEF})
  endif(MSVC)
  add_library(sleid0r_${LOWERCASE_MIXER} MODULE ${${MIXER}_SOURCES})
  set_target_properties (sleid0r_${LOWERCASE_MIXER} PROPERTIES PREFIX "")
  install (TARGETS sleid0r_${LOWERCASE_MIXER} LIBRARY DESTINATION ${LIBDIR})
endforeach()

# set(WIPE_UP_SOURCES          sleid0r_wipe-up.c)
# set(WIPE_LEFT_SOURCES        sleid0r_wipe-left.c)
# set(WIPE_BARN_DOOR_V_SOURCES sleid0r_wipe-barn-door-v.c)
# set(SLIDE_RIGHT_SOURCES      sleid0r_slide-right.c)
# set(PUSH_UP_SOURCES          sleid0r_push-up.c)
# set(PUSH_DOWN_SOURCES        sleid0r_push-down.c)
# set(WIPE_RIGHT_SOURCES       sleid0r_wipe-right.c)
# set(WIPE_DOWN_SOURCES        sleid0r_wipe-down.c)
# set(WIPE_BARN_DOOR_H_SOURCES sleid0r_wipe-barn-door-h.c)
# set(SLIDE_LEFT_SOURCES       sleid0r_slide-left.c)
# set(PUSH_RIGHT_SOURCES       sleid0r_push-right.c)
# set(WIPE_RECT_SOURCES        sleid0r_wipe-rect.c)
# set(WIPE_CIRCLE_SOURCES      sleid0r_wipe-circle.c)
# set(SLIDE_UP_SOURCES         sleid0r_slide-up.c)
# set(SLIDE_DOWN_SOURCES       sleid0r_slide-down.c)
# set(PUSH_LEFT_SOURCES        sleid0r_push-left.c)

# if (MSVC)
#   set(WIPE_UP_SOURCES ${WIPE_UP_SOURCES} ${FREI0R_DEF})
#   set(WIPE_LEFT_SOURCES ${WIPE_LEFT_SOURCES} ${FREI0R_DEF})
#   set(WIPE_BARN_DOOR_V_SOURCES ${WIPE_BARN_DOOR_V_SOURCES} ${FREI0R_DEF})
#   set(SLIDE_RIGHT_SOURCES ${SLIDE_RIGHT_SOURCES} ${FREI0R_DEF})
#   set(PUSH_UP_SOURCES ${PUSH_UP_SOURCES} ${FREI0R_DEF})
#   set(PUSH_DOWN_SOURCES ${PUSH_DOWN_SOURCES} ${FREI0R_DEF})
#   set(WIPE_RIGHT_SOURCES ${WIPE_RIGHT_SOURCES} ${FREI0R_DEF})
#   set(WIPE_DOWN_SOURCES ${WIPE_DOWN_SOURCES} ${FREI0R_DEF})
#   set(WIPE_BARN_DOOR_H_SOURCES ${WIPE_BARN_DOOR_H_SOURCES} ${FREI0R_DEF})
#   set(SLIDE_LEFT_SOURCES ${SLIDE_LEFT_SOURCES} ${FREI0R_DEF})
#   set(PUSH_RIGHT_SOURCES ${PUSH_RIGHT_SOURCES} ${FREI0R_DEF})
#   set(WIPE_RECT_SOURCES ${WIPE_RECT_SOURCES} ${FREI0R_DEF})
#   set(WIPE_CIRCLE_SOURCES ${WIPE_CIRCLE_SOURCES} ${FREI0R_DEF})
#   set(SLIDE_UP_SOURCES ${SLIDE_UP_SOURCES} ${FREI0R_DEF})
#   set(SLIDE_DOWN_SOURCES ${SLIDE_DOWN_SOURCES} ${FREI0R_DEF})
#   set(PUSH_LEFT_SOURCES ${PUSH_LEFT_SOURCES} ${FREI0R_DEF})
# endif (MSVC)


# add_library (alpha0ps MODULE ${O_SOURCES})
# add_library (alphagrad MODULE ${G_SOURCES})
# add_library (alphaspot MODULE ${S_SOURCES})

# if(NOT MSVC)
#   target_link_libraries(alpha0ps -lm)
#   target_link_libraries(alphagrad -lm)
#   target_link_libraries(alphaspot -lm)
# endif()

# set_target_properties (alpha0ps PROPERTIES PREFIX "")
# set_target_properties (alphagrad PROPERTIES PREFIX "")
# set_target_properties (alphaspot PROPERTIES PREFIX "")

# install (TARGETS alpha0ps LIBRARY DESTINATION ${LIBDIR})
# install (TARGETS alphagrad LIBRARY DESTINATION ${LIBDIR})
# install (TARGETS alphaspot LIBRARY DESTINATION ${LIBDIR})
