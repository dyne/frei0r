set (SOURCES flippo.c)

if (MSVC)
  set_source_files_properties (flippo.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (flippo MODULE ${SOURCES})

install (TARGETS flippo LIBRARY DESTINATION lib/frei0r-1)
