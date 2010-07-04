set(CMAKE_C_FLAGS "-std=c99")
set (SOURCES levels.c)

if (MSVC)
  set_source_files_properties (levels.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (levels MODULE ${SOURCES})

install (TARGETS levels LIBRARY DESTINATION lib)
