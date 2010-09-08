set (SOURCES facebl0r.cpp)
set (TARGET facebl0r)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

include_directories (${OpenCV_INCLUDE_DIR})
LINK_LIBRARIES(${OpenCV_LIBS})

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
