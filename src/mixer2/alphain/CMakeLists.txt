set (SOURCES alphain.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (alphain MODULE ${SOURCES})

install (TARGETS alphain LIBRARY DESTINATION lib)
