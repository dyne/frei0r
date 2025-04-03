# add custom target distclean
# cleans and removes cmake generated files etc.
# Jan Woetzel 04/2003
#

if(UNIX)
  add_custom_target(distclean
     COMMENT "cleaning for source distribution"
  )
  set(DISTCLEANED
   cmake.depends
   cmake.check_depends
   CMakeCache.txt
   cmake.check_cache
   *.cmake
   Makefile
   core core.*
   gmon.out
   *~
  )
  add_custom_command(
     COMMENT "running target clean"
     TARGET distclean
     COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
  )
  add_custom_command(
    POST_BUILD
    COMMENT "distribution clean"
    COMMAND rm
    ARGS    -Rf CMakeTmp ${DISTCLEANED}
    TARGET  distclean
  )
endif(UNIX)

