if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES Clang)
  return() # MSVC cl doesn't support Variable Length Arrays (VLA), but clang-cl does
endif()

# Set C99 flag for gcc
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "-std=c99")
endif (CMAKE_COMPILER_IS_GNUCC)

set (SOURCES curves.c)
set (TARGET curves)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
