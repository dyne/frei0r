set (SOURCES alphaout.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (alphaout MODULE ${SOURCES})

install (TARGETS alphaout LIBRARY DESTINATION lib/frei0r-1)
