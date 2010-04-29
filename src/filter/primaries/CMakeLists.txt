set (SOURCES primaries.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (primaries MODULE ${SOURCES})

install (TARGETS primaries LIBRARY DESTINATION lib)
