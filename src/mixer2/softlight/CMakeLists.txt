set (SOURCES softlight.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (softlight MODULE ${SOURCES})

install (TARGETS softlight LIBRARY DESTINATION lib)
