set (SOURCES edgeglow.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (edgeglow MODULE ${SOURCES})

install (TARGETS edgeglow LIBRARY DESTINATION lib)
