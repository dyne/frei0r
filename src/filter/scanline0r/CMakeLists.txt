set (SOURCES scanline0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (scanline0r MODULE ${SOURCES})

install (TARGETS scanline0r LIBRARY DESTINATION lib)
