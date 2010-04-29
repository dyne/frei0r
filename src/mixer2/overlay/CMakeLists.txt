set (SOURCES overlay.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (overlay MODULE ${SOURCES})

install (TARGETS overlay LIBRARY DESTINATION lib)
