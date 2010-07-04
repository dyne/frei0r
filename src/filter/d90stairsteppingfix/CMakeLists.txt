set (SOURCES d90stairsteppingfix.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (d90stairsteppingfix MODULE ${SOURCES})

install (TARGETS d90stairsteppingfix LIBRARY DESTINATION lib/frei0r-1)
