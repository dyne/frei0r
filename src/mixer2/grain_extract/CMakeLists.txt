set (SOURCES grain_extract.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (grain_extract MODULE ${SOURCES})

install (TARGETS grain_extract LIBRARY DESTINATION lib/frei0r-1)
