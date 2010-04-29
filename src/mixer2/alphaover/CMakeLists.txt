set (SOURCES alphaover.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (alphaover MODULE ${SOURCES})

install (TARGETS alphaover LIBRARY DESTINATION lib)
