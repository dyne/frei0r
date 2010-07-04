set (SOURCES delaygrab.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (delaygrab MODULE ${SOURCES})

install (TARGETS delaygrab LIBRARY DESTINATION lib/frei0r-1)
