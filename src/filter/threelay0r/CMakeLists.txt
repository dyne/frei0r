set (SOURCES threelay0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (threelay0r MODULE ${SOURCES})

install (TARGETS threelay0r LIBRARY DESTINATION lib)
