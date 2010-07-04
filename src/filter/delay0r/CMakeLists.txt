set (SOURCES delay0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (delay0r MODULE ${SOURCES})

install (TARGETS delay0r LIBRARY DESTINATION lib/frei0r-1)
