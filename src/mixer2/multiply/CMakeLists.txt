set (SOURCES multiply.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (multiply MODULE ${SOURCES})

install (TARGETS multiply LIBRARY DESTINATION lib)
