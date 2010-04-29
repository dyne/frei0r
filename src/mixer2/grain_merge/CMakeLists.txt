set (SOURCES grain_merge.cpp)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_1_1_DEF})
endif (MSVC)

add_library (grain_merge MODULE ${SOURCES})

install (TARGETS grain_merge LIBRARY DESTINATION lib)
