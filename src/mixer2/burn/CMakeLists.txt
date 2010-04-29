set (SOURCES burn.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (burn MODULE ${SOURCES})

install (TARGETS burn LIBRARY DESTINATION lib)
