set (SOURCES alphaxor.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (alphaxor MODULE ${SOURCES})

install (TARGETS alphaxor LIBRARY DESTINATION lib)
