include_directories (${OpenCV_INCLUDE_DIR})

if (MSVC)
  set_source_files_properties (facedetect.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (${TARGET}  MODULE facedetect.c)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

target_link_libraries (facedetect
  ${OpenCV_LIBS})
install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
