set (SOURCES subtract.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (subtract MODULE ${SOURCES})

install (TARGETS subtract LIBRARY DESTINATION lib)
