set (SOURCES defish0r.c interp.h)

if (MSVC)
  set_source_files_properties (defish0r.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (defish0r MODULE ${SOURCES})

install (TARGETS defish0r LIBRARY DESTINATION lib)
