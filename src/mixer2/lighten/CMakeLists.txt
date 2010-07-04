set (SOURCES lighten.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (lighten MODULE ${SOURCES})

install (TARGETS lighten LIBRARY DESTINATION lib/frei0r-1)
