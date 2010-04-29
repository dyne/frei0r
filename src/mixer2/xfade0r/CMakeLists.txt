set (SOURCES xfade0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (xfade0r MODULE ${SOURCES})

install (TARGETS xfade0r LIBRARY DESTINATION lib)
