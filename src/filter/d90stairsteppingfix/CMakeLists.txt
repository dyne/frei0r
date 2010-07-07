set (SOURCES d90stairsteppingfix.cpp)
set (TARGET d90stairsteppingfix)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET} MODULE ${SOURCES})

# No «lib» prefix (name.so instead of libname.so)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
