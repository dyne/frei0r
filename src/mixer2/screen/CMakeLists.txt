set (SOURCES screen.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (screen MODULE ${SOURCES})

install (TARGETS screen LIBRARY DESTINATION lib)
