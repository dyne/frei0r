set (SOURCES partik0l.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (partik0l MODULE ${SOURCES})

install (TARGETS partik0l LIBRARY DESTINATION lib)
