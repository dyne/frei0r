set (SOURCES three_point_balance.c)

if (MSVC)
  set_source_files_properties (three_point_balance.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (three_point_balance MODULE ${SOURCES})

install (TARGETS three_point_balance LIBRARY DESTINATION lib)
