set (SOURCES nervous.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (nervous MODULE ${SOURCES})

install (TARGETS nervous LIBRARY DESTINATION lib)
