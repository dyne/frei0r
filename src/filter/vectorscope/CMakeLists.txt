if (MSVC)
  set_source_files_properties (vectorscope.c PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library (vectorscope MODULE vectorscope.c vectorscope_image.h)

install (TARGETS vectorscope LIBRARY DESTINATION lib)
