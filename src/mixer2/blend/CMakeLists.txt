set (SOURCES blend.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (blend MODULE ${SOURCES})

install (TARGETS blend LIBRARY DESTINATION lib/frei0r-1)
