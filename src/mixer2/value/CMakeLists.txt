set (SOURCES value.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (value MODULE ${SOURCES})

install (TARGETS value LIBRARY DESTINATION lib/frei0r-1)
