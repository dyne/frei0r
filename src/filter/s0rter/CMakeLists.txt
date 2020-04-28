if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "-Wall -Werror -Wextra")
endif (CMAKE_COMPILER_IS_GNUCC)
set(SOURCES s0rter.c s0rter.h)
set(TARGET s0rter)

if (MSVC)
  set_source_files_properties(s0rter.c PROPERTIES LANGUAGE CXX)
  set(SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

link_libraries(m)
add_library(${TARGET} MODULE ${SOURCES})
set_target_properties(${TARGET} PROPERTIES PREFIX "")

install(TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})

