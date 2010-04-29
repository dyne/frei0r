set (SOURCES cartoon.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (cartoon MODULE ${SOURCES})

install (TARGETS cartoon LIBRARY DESTINATION lib)
