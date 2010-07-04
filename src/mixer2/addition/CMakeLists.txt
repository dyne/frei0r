set (SOURCES addition.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (addition MODULE ${SOURCES})

install (TARGETS addition LIBRARY DESTINATION lib/frei0r-1)
