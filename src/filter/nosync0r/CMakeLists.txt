set (SOURCES nosync0r.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (nosync0r MODULE ${SOURCES})

install (TARGETS nosync0r LIBRARY DESTINATION lib)
