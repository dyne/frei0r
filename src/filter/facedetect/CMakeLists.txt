set (SOURCES facedetect.cpp)
set (TARGET facedetect)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

include_directories(${OpenCV_INCLUDE_DIRS})
add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")
target_link_libraries(${TARGET} ${OpenCV_LIBS})

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
