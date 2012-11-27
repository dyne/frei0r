set (SOURCES cairoaffineblend.c)
set (TARGET cairoaffineblend)

include_directories(${Cairo_INCLUDE_DIR})
set(LIBS ${LIBS} ${Cairo_LIBRARY})

if (MSVC)
  set_source_files_properties (cairoaffineblend.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")
target_link_libraries(cairoaffineblend ${LIBS})

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
