# Audit selected bundle object files for duplicate strong external symbols.
# CMAKE_NM is supplied by the configured toolchain so LLVM/Wasm builds use
# their matching nm instead of a host-specific executable.

if(NOT DEFINED NM OR NM STREQUAL "")
  message(STATUS "SKIP: no CMake-selected nm is available")
  return()
endif()
if(NOT DEFINED MANIFESTS OR MANIFESTS STREQUAL "")
  message(FATAL_ERROR "symbol audit requires at least one target manifest")
endif()

set(approved_symbol_patterns
  "^(__.*|_GLOBAL_OFFSET_TABLE_|DW\\.ref\\.).*$"
  "^[A-Za-z0-9_]+_f0r_(init|deinit|get_plugin_info|get_param_info|construct|destruct|set_param_value|get_param_value|update|update2)$"
  "^f0r_bundle_descriptor_[A-Za-z0-9_]+$")

# kaleid0sc0pe deliberately has a private C++ interface shared by api.cpp and
# kaleid0sc0pe.cpp.  The private header gives it this bundle-only namespace;
# these are the sole non-runtime cross-translation-unit definitions allowed.
set(approved_cross_translation_unit_pattern "^f0r_bundle_kaleid0sc0pe::")
set(approved_ntsc_cross_translation_unit_pattern
  "^crt_(demodulate|init|modulate|reset|resize|sincos14)$")

string(REPLACE "," ";" manifests "${MANIFESTS}")
set(definitions)
foreach(manifest_spec IN LISTS manifests)
  string(FIND "${manifest_spec}" "=" separator)
  if(separator EQUAL -1)
    message(FATAL_ERROR "invalid symbol audit manifest '${manifest_spec}'")
  endif()
  string(SUBSTRING "${manifest_spec}" 0 ${separator} owner)
  math(EXPR path_start "${separator} + 1")
  string(SUBSTRING "${manifest_spec}" ${path_start} -1 manifest)
  if(NOT EXISTS "${manifest}")
    message(FATAL_ERROR "symbol audit manifest for ${owner} was not generated: ${manifest}")
  endif()
  file(STRINGS "${manifest}" objects)
  foreach(object IN LISTS objects)
    if(NOT EXISTS "${object}")
      message(FATAL_ERROR "symbol audit object for ${owner} is missing: ${object}")
    endif()
    execute_process(
      COMMAND "${NM}" --defined-only --extern-only --format=posix --demangle "${object}"
      RESULT_VARIABLE nm_result
      OUTPUT_VARIABLE nm_output
      ERROR_VARIABLE nm_error
    )
    if(NOT nm_result EQUAL 0)
      string(STRIP "${nm_error}" nm_reason)
      if(nm_reason STREQUAL "")
        set(nm_reason "nm rejected the requested portable output format")
      endif()
      message(STATUS "SKIP: symbol audit unsupported by ${NM}: ${nm_reason}")
      return()
    endif()
    string(REPLACE "\n" ";" nm_lines "${nm_output}")
    set(parsed_symbol FALSE)
    foreach(line IN LISTS nm_lines)
      # POSIX nm is '<name> <type> <value> ...'; the name may be demangled.
      string(REGEX MATCH "^(.+) ([A-Z]) [0-9A-Fa-f]+" symbol_match "${line}")
      if(NOT symbol_match)
        continue()
      endif()
      set(parsed_symbol TRUE)
      set(symbol "${CMAKE_MATCH_1}")
      set(type "${CMAKE_MATCH_2}")
      # U is undefined; V/W are weak.  The remaining uppercase definitions are
      # link-visible definitions that can collide in a static or shared bundle.
      if(type STREQUAL "U" OR type STREQUAL "V" OR type STREQUAL "W")
        continue()
      endif()
      set(approved FALSE)
      foreach(pattern IN LISTS approved_symbol_patterns)
        if(symbol MATCHES "${pattern}")
          set(approved TRUE)
          break()
        endif()
      endforeach()
      if(owner STREQUAL "frei0r-bundle-object-kaleid0sc0pe" AND
         (symbol MATCHES "${approved_cross_translation_unit_pattern}" OR
          symbol MATCHES "^std::default_delete<f0r_bundle_kaleid0sc0pe::" OR
          symbol MATCHES "^typeinfo( name)? for f0r_bundle_kaleid0sc0pe::" OR
          symbol MATCHES "^vtable for f0r_bundle_kaleid0sc0pe::"))
        set(approved TRUE)
      endif()
      # crt_core.c and crt_ntsc.c share this explicit private header API.
      if(owner STREQUAL "frei0r-bundle-object-ntsc" AND
         symbol MATCHES "${approved_ntsc_cross_translation_unit_pattern}")
        set(approved TRUE)
      endif()
      if(NOT approved)
        list(APPEND definitions "${symbol}|${owner}")
      endif()
    endforeach()
    string(STRIP "${nm_output}" stripped_nm_output)
    if(NOT stripped_nm_output STREQUAL "" AND NOT parsed_symbol)
      message(STATUS "SKIP: symbol audit unsupported by ${NM}: output is not POSIX nm format")
      return()
    endif()
  endforeach()
endforeach()

set(symbols)
foreach(definition IN LISTS definitions)
  string(FIND "${definition}" "|" separator)
  string(SUBSTRING "${definition}" 0 ${separator} symbol)
  list(APPEND symbols "${symbol}")
endforeach()
list(REMOVE_DUPLICATES symbols)
list(SORT symbols)

set(collisions)
foreach(symbol IN LISTS symbols)
  set(owners)
  foreach(definition IN LISTS definitions)
    string(FIND "${definition}" "|" separator)
    string(SUBSTRING "${definition}" 0 ${separator} definition_symbol)
    if(definition_symbol STREQUAL symbol)
      math(EXPR owner_start "${separator} + 1")
      string(SUBSTRING "${definition}" ${owner_start} -1 owner)
      list(APPEND owners "${owner}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES owners)
  list(LENGTH owners definition_count)
  if(definition_count GREATER 1)
    list(SORT owners)
    string(REPLACE ";" ", " owners_text "${owners}")
    string(APPEND collisions "  ${symbol}: ${owners_text}\n")
  endif()
endforeach()

set(unapproved)
foreach(definition IN LISTS definitions)
  string(FIND "${definition}" "|" separator)
  string(SUBSTRING "${definition}" 0 ${separator} symbol)
  math(EXPR owner_start "${separator} + 1")
  string(SUBSTRING "${definition}" ${owner_start} -1 owner)
  string(APPEND unapproved "  ${symbol}: ${owner}\n")
endforeach()

if(collisions OR unapproved)
  message(FATAL_ERROR
    "unapproved strong external symbols in bundle objects:\n${unapproved}"
    "duplicate definitions (if any):\n${collisions}")
endif()
message(STATUS "bundle external-symbol audit passed for ${NM}")
