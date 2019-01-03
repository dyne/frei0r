set (SOURCES facedetect.cpp)
set (TARGET facedetect)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

include_directories (${OpenCV_INCLUDE_DIRS})
link_directories (${OpenCV_LIBRARY_DIRS})
LINK_LIBRARIES(${OpenCV_LIBS} ${OpenCV_LIBRARIES})

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
