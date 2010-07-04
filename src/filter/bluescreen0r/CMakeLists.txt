set (SOURCES bluescreen0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (bluescreen0r MODULE ${SOURCES})

install (TARGETS bluescreen0r LIBRARY DESTINATION lib/frei0r-1)
