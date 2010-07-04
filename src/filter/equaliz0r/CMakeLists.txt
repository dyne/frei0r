set (SOURCES equaliz0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (equaliz0r MODULE ${SOURCES})

install (TARGETS equaliz0r LIBRARY DESTINATION lib/frei0r-1)
