file(REMOVE_RECURSE "${WORK_DIR}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${WORK_DIR}"
    -DFREI0R_BUILD_BUNDLE=OFF -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON
    -DWITHOUT_GAVL=ON
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "default-off configure failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${WORK_DIR}" --target frei0r-bundle
  RESULT_VARIABLE build_result
)
if(build_result EQUAL 0)
  message(FATAL_ERROR "default build unexpectedly contains frei0r-bundle")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${WORK_DIR}" --target brightness
  RESULT_VARIABLE module_build_result
)
if(NOT module_build_result EQUAL 0)
  message(FATAL_ERROR "ordinary brightness module build failed")
endif()
