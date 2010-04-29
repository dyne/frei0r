set (SOURCES dodge.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (dodge MODULE ${SOURCES})

install (TARGETS dodge LIBRARY DESTINATION lib)
