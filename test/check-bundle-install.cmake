if(NOT DEFINED BUNDLE_BUILD_DIR OR NOT DEFINED CONSUMER_SOURCE_DIR OR
   NOT DEFINED PREFIX OR NOT DEFINED INSTALL_LIBDIR)
  message(FATAL_ERROR "bundle install test requires build, consumer, prefix, and libdir paths")
endif()

file(REMOVE_RECURSE "${PREFIX}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${BUNDLE_BUILD_DIR}" --prefix "${PREFIX}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "bundle installation failed: ${install_result}")
endif()

set(required_files
  "${PREFIX}/include/frei0r/bundle.h"
  "${PREFIX}/${INSTALL_LIBDIR}/${STATIC_LIBRARY_PREFIX}frei0r-bundle-static${STATIC_LIBRARY_SUFFIX}"
  "${PREFIX}/${INSTALL_LIBDIR}/${SHARED_LIBRARY_PREFIX}frei0r-bundle${SHARED_LIBRARY_SUFFIX}"
  "${PREFIX}/${INSTALL_LIBDIR}/cmake/Frei0rBundle/Frei0rBundleConfig.cmake"
  "${PREFIX}/${INSTALL_LIBDIR}/cmake/Frei0rBundle/Frei0rBundleTargets.cmake"
)
foreach(required_file IN LISTS required_files)
  if(NOT EXISTS "${required_file}")
    message(FATAL_ERROR "missing installed bundle interface: ${required_file}")
  endif()
endforeach()

set(consumer_build_dir "${PREFIX}/consumer-build")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${CONSUMER_SOURCE_DIR}" -B "${consumer_build_dir}"
    "-DCMAKE_PREFIX_PATH=${PREFIX}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "installed bundle consumer configuration failed: ${configure_result}")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${consumer_build_dir}"
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "installed bundle consumer build failed: ${build_result}")
endif()
foreach(consumer IN ITEMS c-static c-shared cxx-static cxx-shared)
  execute_process(
    COMMAND "${consumer_build_dir}/${consumer}"
    RESULT_VARIABLE consumer_result
  )
  if(NOT consumer_result EQUAL 0)
    message(FATAL_ERROR "installed bundle ${consumer} failed: ${consumer_result}")
  endif()
endforeach()
