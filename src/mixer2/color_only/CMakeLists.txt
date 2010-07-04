set (SOURCES color_only.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (color_only MODULE ${SOURCES})

install (TARGETS color_only LIBRARY DESTINATION lib/frei0r-1)
