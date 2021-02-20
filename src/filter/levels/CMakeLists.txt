# Set C99 flag for gcc
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "-std=c99")
endif (CMAKE_COMPILER_IS_GNUCC)

set (SOURCES levels.c)
set (TARGET levels)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
