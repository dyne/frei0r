set (SOURCES divide.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (divide MODULE ${SOURCES})

install (TARGETS divide LIBRARY DESTINATION lib/frei0r-1)
