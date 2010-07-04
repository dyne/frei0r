set (SOURCES baltan.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (baltan MODULE ${SOURCES})

install (TARGETS baltan LIBRARY DESTINATION lib/frei0r-1)
