set (SOURCES darken.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (darken MODULE ${SOURCES})

install (TARGETS darken LIBRARY DESTINATION lib)
