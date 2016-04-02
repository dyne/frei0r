set (SOURCES hqdn3d.c)
set (TARGET hqdn3d)

if (MSVC)
  set_source_files_properties (hqdn3d.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

link_libraries(m)
add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
