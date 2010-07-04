set (SOURCES plasma.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (plasma MODULE ${SOURCES})

install (TARGETS plasma LIBRARY DESTINATION lib/frei0r-1)
