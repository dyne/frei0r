set (SOURCES nois0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (nois0r MODULE ${SOURCES})

install (TARGETS nois0r LIBRARY DESTINATION lib/frei0r-1)
