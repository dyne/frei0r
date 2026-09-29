file(REMOVE_RECURSE "${WORK_DIR}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/test/bundle-duplicate-registry"
    -B "${WORK_DIR}" -DFREI0R_SOURCE_DIR=${SOURCE_DIR}
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "duplicate registry fixture configure failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${WORK_DIR}" --target bundle-duplicate-registry
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "duplicate registry fixture build failed")
endif()
execute_process(
  COMMAND "${WORK_DIR}/bundle-duplicate-registry"
  RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "generated registry did not reject duplicate ID")
endif()
