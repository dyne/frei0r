# facedetect filter - requires OpenCV (and protobuf)
# This filter can cause conflicts with applications that also use protobuf
# such as MLT, so it can be disabled with -DWITHOUT_FACERECOGNITION=ON

if (OpenCV_FOUND AND NOT WITHOUT_FACERECOGNITION)
  set (SOURCES facedetect.cpp)
  set (TARGET facedetect)

  if (MSVC)
    set (SOURCES ${SOURCES} ${FREI0R_DEF})
  endif (MSVC)

  include_directories(${OpenCV_INCLUDE_DIRS})
  add_library (${TARGET} MODULE ${SOURCES})
  set_target_properties (${TARGET} PROPERTIES PREFIX "")
  target_link_libraries(${TARGET} ${OpenCV_LIBS})

  install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
endif()
