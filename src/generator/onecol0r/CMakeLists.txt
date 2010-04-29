set (SOURCES onecol0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (onecol0r MODULE ${SOURCES})

install (TARGETS onecol0r LIBRARY DESTINATION lib)
