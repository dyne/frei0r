set (SOURCES twolay0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (twolay0r MODULE ${SOURCES})

install (TARGETS twolay0r LIBRARY DESTINATION lib)
