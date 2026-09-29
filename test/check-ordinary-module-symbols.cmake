execute_process(
  COMMAND "${NM}" -D --defined-only "${PROBE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE symbols
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "could not inspect ${PROBE}")
endif()
foreach(symbol f0r_init f0r_deinit f0r_get_plugin_info f0r_get_param_info f0r_construct f0r_destruct f0r_set_param_value f0r_get_param_value f0r_update)
  string(FIND "${symbols}" " ${symbol}" exported)
  if(exported EQUAL -1)
    message(FATAL_ERROR "ordinary module did not export ${symbol}")
  endif()
endforeach()
