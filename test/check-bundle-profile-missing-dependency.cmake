execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/test/bundle-profile-missing-dependency"
    -B "${WORK_DIR}"
  RESULT_VARIABLE configure_result
  OUTPUT_VARIABLE configure_output
  ERROR_VARIABLE configure_error)
if(configure_result EQUAL 0)
  message(FATAL_ERROR "missing-dependency fixture unexpectedly configured")
endif()
set(configure_report "${configure_output}\n${configure_error}")
string(FIND "${configure_report}" "requires unavailable bundle dependency" missing_reason)
string(FIND "${configure_report}" "TEST_MISSING_DEPENDENCY" missing_name)
if(missing_reason EQUAL -1 OR missing_name EQUAL -1)
  message(FATAL_ERROR "missing-dependency failure was not actionable: ${configure_report}")
endif()
