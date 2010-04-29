if (MSVC)
  set_source_files_properties (rgbparade.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (rgbparade MODULE rgbparade.c rgbparade_image.h)

install (TARGETS rgbparade LIBRARY DESTINATION lib)
