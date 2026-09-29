execute_process(
  COMMAND "${CMAKE_COMMAND}" -DNM=${NM} -DMANIFESTS=${MANIFESTS}
    -P "${AUDIT_SCRIPT}"
  RESULT_VARIABLE audit_result
  OUTPUT_VARIABLE audit_output
  ERROR_VARIABLE audit_error
)
if(audit_result EQUAL 0)
  message(FATAL_ERROR "intentional duplicate fixture unexpectedly passed the symbol audit")
endif()
set(audit_report "${audit_output}\n${audit_error}")
foreach(expected intentional_bundle_symbol_collision symbol-collision-fixture-a symbol-collision-fixture-b)
  string(FIND "${audit_report}" "${expected}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "symbol audit did not report ${expected}: ${audit_report}")
  endif()
endforeach()
