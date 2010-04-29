set (SOURCES sobel.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (sobel MODULE ${SOURCES})

install (TARGETS sobel LIBRARY DESTINATION lib)
