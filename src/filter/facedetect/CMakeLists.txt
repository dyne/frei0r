include_directories (${OpenCV_INCLUDE_DIR})

if (MSVC)
  set_source_files_properties (facedetect.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (facedetect MODULE facedetect.c)

target_link_libraries (facedetect
  ${OpenCV_LIBS})
install (TARGETS facedetect LIBRARY DESTINATION lib/frei0r-1)
