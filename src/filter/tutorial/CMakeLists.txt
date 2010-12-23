# Replace «tutorial» by the library name of your filter
# and add the filter directory in src/filter/CMakeLists.txt.
set (SOURCES tutorial.cpp)
set (TARGET tutorial)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})

# No «lib» prefix (name.so instead of libname.so)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
