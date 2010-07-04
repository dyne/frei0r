set (SOURCES water.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (water MODULE ${SOURCES})

install (TARGETS water LIBRARY DESTINATION lib/frei0r-1)
