set (SOURCES hue.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (hue MODULE ${SOURCES})

install (TARGETS hue LIBRARY DESTINATION lib)
