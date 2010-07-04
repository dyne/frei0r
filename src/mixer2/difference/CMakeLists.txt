set (SOURCES difference.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (difference MODULE ${SOURCES})

install (TARGETS difference LIBRARY DESTINATION lib/frei0r-1)
