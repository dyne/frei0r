set (SOURCES coloradj_RGB.c)
set (TARGET coloradj_RGB)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

if(NOT MSVC)
  link_libraries(m)
endif()
add_library (${TARGET} MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
