set (SOURCES cairoimagegrid.c)
set (TARGET cairoimagegrid)

include_directories(${Cairo_INCLUDE_DIR})
set(LIBS ${LIBS} ${Cairo_LIBRARY})

if (MSVC)
  set_source_files_properties (cairoimagegrid.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")
target_link_libraries(cairoimagegrid ${LIBS})

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
