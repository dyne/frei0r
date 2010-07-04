set (SOURCES host_param_test.c)

if (MSVC)
  set_source_files_properties (host_param_test.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (host_param_test MODULE ${SOURCES})

install (TARGETS host_param_test LIBRARY DESTINATION lib/frei0r-1)
