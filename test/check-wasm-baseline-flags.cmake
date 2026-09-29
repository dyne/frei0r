if(NOT DEFINED COMPILE_COMMANDS)
  message(FATAL_ERROR "COMPILE_COMMANDS is required")
endif()
if(NOT EXISTS "${COMPILE_COMMANDS}")
  message(FATAL_ERROR "missing compile commands file: ${COMPILE_COMMANDS}")
endif()

file(READ "${COMPILE_COMMANDS}" commands)
string(REGEX MATCHALL "frei0r-bundle-object-" bundle_entries "${commands}")
list(LENGTH bundle_entries bundle_count)
if(bundle_count LESS 20)
  message(FATAL_ERROR "expected a complete core bundle compile database, found ${bundle_count} bundle objects")
endif()

string(REGEX MATCHALL "[{][^}]*frei0r-bundle-object-[^}]*[}]"
  bundle_commands "${commands}")

foreach(forbidden "-msse" "-mavx" "-pthread" "USE_PTHREADS")
  string(FIND "${bundle_commands}" "${forbidden}" forbidden_offset)
  if(NOT forbidden_offset EQUAL -1)
    message(FATAL_ERROR
      "x86 SIMD or pthread flag '${forbidden}' leaked into the scalar Wasm baseline")
  endif()
endforeach()

foreach(required "frei0r-bundle-object-kaleid0sc0pe" "-DNO_SSE2" "-DNO_FUTURE"
                 "frei0r-bundle-object-squigglevision")
  string(FIND "${commands}" "${required}" required_offset)
  if(required_offset EQUAL -1)
    message(FATAL_ERROR "scalar Wasm baseline is missing '${required}'")
  endif()
endforeach()
