set(CMAKE_C_FLAGS "-std=c99")
set (SOURCES curves.c)

if (MSVC)
  set_source_files_properties (curves.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (curves MODULE ${SOURCES})

install (TARGETS curves LIBRARY DESTINATION lib)
