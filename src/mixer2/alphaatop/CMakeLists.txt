set (SOURCES alphaatop.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (alphaatop MODULE ${SOURCES})

install (TARGETS alphaatop LIBRARY DESTINATION lib/frei0r-1)
