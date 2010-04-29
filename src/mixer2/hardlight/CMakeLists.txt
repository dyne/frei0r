set (SOURCES hardlight.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (hardlight MODULE ${SOURCES})

install (TARGETS hardlight LIBRARY DESTINATION lib)
