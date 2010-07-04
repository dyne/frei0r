set (SOURCES RGB.c)

if (MSVC)
  set_source_files_properties (RGB.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (RGB MODULE ${SOURCES})

install (TARGETS RGB LIBRARY DESTINATION lib/frei0r-1)
