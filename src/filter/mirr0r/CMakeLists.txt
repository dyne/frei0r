set (SOURCES mirr0r.cpp)
set (TARGET mirr0r)

include_directories(${Cairo_INCLUDE_DIR})
set(LIBS ${LIBS} ${Cairo_LIBRARY})

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})

set_target_properties (${TARGET} PROPERTIES PREFIX "")
target_link_libraries(mirr0r ${LIBS})
install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
