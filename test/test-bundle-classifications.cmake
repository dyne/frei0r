set(check_script "${CMAKE_CURRENT_LIST_DIR}/check-bundle-classifications.cmake")

function(check_case name targets classifications expected_error)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DCLASSIFICATION_TARGETS=${targets}"
      "-DCLASSIFICATIONS=${classifications}"
      -P "${check_script}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  if(expected_error STREQUAL "")
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "${name} should pass: ${output}\n${error}")
    endif()
  elseif(result EQUAL 0 OR NOT error MATCHES "${expected_error}")
    message(FATAL_ERROR "${name} did not report '${expected_error}': ${output}\n${error}")
  endif()
endfunction()

set(brightness "brightness|core|language runtime and math library only")
set(flarefx "flarefx|core|language runtime and math library only")
set(unexpected "unexpected|core|language runtime and math library only")

check_case(valid "brightness" "${brightness}" "")
check_case(merged_plugin "brightness;flarefx" "${brightness};${flarefx}" "")
check_case(reordered "flarefx;brightness" "${brightness};${flarefx}" "")
check_case(missing "brightness;flarefx" "${brightness}" "missing classifications")
check_case(duplicate "brightness" "${brightness};${brightness}" "duplicate classification")
check_case(wrong_target_same_count "brightness;flarefx"
  "${brightness};${unexpected}" "unexpected or duplicate classification")
check_case(malformed "brightness" "brightness||reason" "malformed target classification")
check_case(duplicate_inventory "brightness;brightness" "${brightness}"
  "duplicate target in the classification inventory")
