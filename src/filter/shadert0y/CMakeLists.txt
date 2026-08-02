find_package(OpenGL REQUIRED)
find_package(EGL REQUIRED)

set (SOURCES shadert0y.cpp)
set (TARGET shadert0y)

include_directories(${OPENGL_INCLUDE_DIR} ${EGL_INCLUDE_DIR})

add_library (${TARGET} MODULE ${SOURCES})

set_target_properties (${TARGET} PROPERTIES PREFIX "")

target_link_libraries(${TARGET} ${OPENGL_LIBRARIES} ${EGL_LIBRARIES})

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
