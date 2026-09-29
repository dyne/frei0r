execute_process(
  COMMAND "${NM}" -D --defined-only "${PROBE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE symbols
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "could not inspect ${PROBE}")
endif()
string(REGEX MATCH "[ \t]f0r_(init|deinit|get_plugin_info|get_param_info|construct|destruct|set_param_value|get_param_value|update|update2)(\\n|$)" leaked "${symbols}")
if(leaked)
  message(FATAL_ERROR "bundle exported a plugin entry point: ${leaked}")
endif()
foreach(symbol f0r_bundle_plugin_count f0r_bundle_plugin_by_index f0r_bundle_plugin_by_id)
  string(FIND "${symbols}" " ${symbol}" exported)
  if(exported EQUAL -1)
    message(FATAL_ERROR "bundle did not export registry API ${symbol}")
  endif()
endforeach()
