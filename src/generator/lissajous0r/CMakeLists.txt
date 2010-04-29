set (SOURCES lissajous0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (lissajous0r MODULE ${SOURCES})

install (TARGETS lissajous0r LIBRARY DESTINATION lib)
