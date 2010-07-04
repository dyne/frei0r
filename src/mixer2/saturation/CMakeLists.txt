set (SOURCES saturation.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (saturation MODULE ${SOURCES})

install (TARGETS saturation LIBRARY DESTINATION lib/frei0r-1)
