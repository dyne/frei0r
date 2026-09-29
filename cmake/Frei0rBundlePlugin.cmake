# Reusable descriptor generation for isolated frei0r bundle plugin objects.

include(CMakeParseArguments)

set(FREI0R_BUNDLE_PLUGIN_DESCRIPTOR_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/Frei0rBundlePluginDescriptor.c.in")
set(FREI0R_BUNDLE_REGISTRY_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/Frei0rBundleRegistry.c.in")

function(frei0r_bundle_sanitize_id input output)
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" sanitized "${input}")
  if(sanitized MATCHES "^[0-9]" OR sanitized STREQUAL "")
    set(sanitized "plugin_${sanitized}")
  endif()
  set(${output} "${sanitized}" PARENT_SCOPE)
endfunction()

function(frei0r_bundle_register_id id)
  set(collection default)
  if(ARGC GREATER 1)
    set(collection "${ARGV1}")
  endif()
  frei0r_bundle_sanitize_id("${id}" sanitized)
  get_property(registered GLOBAL PROPERTY FREI0R_BUNDLE_SANITIZED_IDS_${collection})
  list(FIND registered "${sanitized}" duplicate_index)
  if(NOT duplicate_index EQUAL -1)
    message(FATAL_ERROR
      "frei0r bundle ID '${id}' collides after sanitization as '${sanitized}'")
  endif()
  set_property(GLOBAL APPEND PROPERTY FREI0R_BUNDLE_SANITIZED_IDS_${collection} "${sanitized}")
endfunction()

# Create a generated, immutable descriptor for one already-isolated plugin
# target.  UPDATE and UPDATE2 state whether that plugin provides each optional
# entry point; the other eight frei0r 1.2 entry points are always present.
function(frei0r_bundle_add_descriptor)
  cmake_parse_arguments(ARG "" "TARGET;ID;UPDATE;UPDATE2;OUT_TARGET;COLLECTION" ""
                        ${ARGN})
  foreach(required TARGET ID OUT_TARGET)
    if(NOT DEFINED ARG_${required} OR ARG_${required} STREQUAL "")
      message(FATAL_ERROR "frei0r_bundle_add_descriptor requires ${required}")
    endif()
  endforeach()
  if(NOT TARGET ${ARG_TARGET})
    message(FATAL_ERROR "bundle plugin target '${ARG_TARGET}' does not exist")
  endif()

  if(NOT ARG_COLLECTION)
    set(ARG_COLLECTION default)
  endif()
  frei0r_bundle_register_id("${ARG_ID}" "${ARG_COLLECTION}")
  frei0r_bundle_sanitize_id("${ARG_ID}" token)
  set(FREI0R_BUNDLE_DESCRIPTOR_SYMBOL "f0r_bundle_descriptor_${token}")
  set(FREI0R_BUNDLE_DESCRIPTOR_ID "${ARG_ID}")
  if(ARG_UPDATE)
    set(FREI0R_BUNDLE_DESCRIPTOR_UPDATE f0r_update)
  else()
    set(FREI0R_BUNDLE_DESCRIPTOR_UPDATE NULL)
  endif()
  if(ARG_UPDATE2)
    set(FREI0R_BUNDLE_DESCRIPTOR_UPDATE2 f0r_update2)
  else()
    set(FREI0R_BUNDLE_DESCRIPTOR_UPDATE2 NULL)
  endif()

  set(descriptor_source
      "${CMAKE_CURRENT_BINARY_DIR}/bundle-descriptors/${token}-descriptor.c")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bundle-descriptors")
  configure_file("${FREI0R_BUNDLE_PLUGIN_DESCRIPTOR_TEMPLATE}"
                 "${descriptor_source}" @ONLY)
  add_library(${ARG_OUT_TARGET} OBJECT "${descriptor_source}")
  target_compile_definitions(${ARG_TARGET} PRIVATE
    FREI0R_BUNDLE_PLUGIN_TOKEN=${token}
  )
  target_compile_definitions(${ARG_OUT_TARGET} PRIVATE
    FREI0R_BUNDLE_PLUGIN_TOKEN=${token}
  )
  set_target_properties(${ARG_TARGET} ${ARG_OUT_TARGET} PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
  )
  set(${ARG_OUT_TARGET}_SYMBOL "${FREI0R_BUNDLE_DESCRIPTOR_SYMBOL}" PARENT_SCOPE)
endfunction()

# Metadata for the small initial bundle profile.  The declaration creates the
# normal frei0r MODULE unchanged and records enough private information for a
# separately compiled bundle object when explicitly selected.
function(frei0r_add_plugin)
  cmake_parse_arguments(ARG "BUNDLE_ELIGIBLE"
    "NAME;KIND;BUNDLE_ID;UPDATE;UPDATE2;MSVC_DEFINITION"
    "SOURCES;COMPILE_DEFINITIONS;COMPILE_OPTIONS;LINK_LIBRARIES" ${ARGN})
  foreach(required NAME KIND SOURCES)
    if(NOT ARG_${required})
      message(FATAL_ERROR "frei0r_add_plugin requires ${required}")
    endif()
  endforeach()
  if(NOT ARG_BUNDLE_ID)
    set(ARG_BUNDLE_ID "${ARG_NAME}")
  endif()

  set(bundle_sources)
  foreach(source IN LISTS ARG_SOURCES)
    if(IS_ABSOLUTE "${source}")
      list(APPEND bundle_sources "${source}")
    else()
      list(APPEND bundle_sources "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    endif()
  endforeach()
  set(module_sources ${ARG_SOURCES})
  if(MSVC AND ARG_MSVC_DEFINITION)
    list(APPEND module_sources "${ARG_MSVC_DEFINITION}")
  endif()
  add_library(${ARG_NAME} MODULE ${module_sources})
  set_target_properties(${ARG_NAME} PROPERTIES PREFIX "")
  if(ARG_COMPILE_DEFINITIONS)
    target_compile_definitions(${ARG_NAME} PRIVATE ${ARG_COMPILE_DEFINITIONS})
  endif()
  if(ARG_COMPILE_OPTIONS)
    target_compile_options(${ARG_NAME} PRIVATE ${ARG_COMPILE_OPTIONS})
  endif()
  if(ARG_LINK_LIBRARIES)
    target_link_libraries(${ARG_NAME} PRIVATE ${ARG_LINK_LIBRARIES})
  endif()
  install(TARGETS ${ARG_NAME} LIBRARY DESTINATION ${LIBDIR})

  set_target_properties(${ARG_NAME} PROPERTIES
    FREI0R_PLUGIN_KIND "${ARG_KIND}"
    FREI0R_PLUGIN_BUNDLE_ID "${ARG_BUNDLE_ID}"
    FREI0R_PLUGIN_BUNDLE_ELIGIBLE "${ARG_BUNDLE_ELIGIBLE}"
    FREI0R_PLUGIN_SOURCES "${bundle_sources}"
    FREI0R_PLUGIN_UPDATE "${ARG_UPDATE}"
    FREI0R_PLUGIN_UPDATE2 "${ARG_UPDATE2}"
    FREI0R_PLUGIN_COMPILE_DEFINITIONS "${ARG_COMPILE_DEFINITIONS}"
    FREI0R_PLUGIN_COMPILE_OPTIONS "${ARG_COMPILE_OPTIONS}"
    FREI0R_PLUGIN_LINK_LIBRARIES "${ARG_LINK_LIBRARIES}"
  )
  set_property(GLOBAL APPEND PROPERTY FREI0R_PLUGIN_TARGETS ${ARG_NAME})
endfunction()

function(frei0r_finalize_bundle)
  if(NOT FREI0R_BUILD_BUNDLE)
    return()
  endif()
  if(NOT FREI0R_BUNDLE_PLUGINS)
    message(FATAL_ERROR "FREI0R_BUILD_BUNDLE requires FREI0R_BUNDLE_PLUGINS")
  endif()

  set(bundle_objects)
  set(bundle_descriptor_objects)
  set(bundle_registry_declarations)
  set(bundle_registry_entries)
  set(bundle_link_libraries)
  foreach(plugin IN LISTS FREI0R_BUNDLE_PLUGINS)
    if(NOT TARGET ${plugin})
      message(FATAL_ERROR "FREI0R_BUNDLE_PLUGINS selects unknown plugin '${plugin}'")
    endif()
    get_target_property(eligible ${plugin} FREI0R_PLUGIN_BUNDLE_ELIGIBLE)
    if(NOT eligible)
      message(FATAL_ERROR "plugin '${plugin}' is not bundle eligible")
    endif()
    get_target_property(sources ${plugin} FREI0R_PLUGIN_SOURCES)
    get_target_property(id ${plugin} FREI0R_PLUGIN_BUNDLE_ID)
    get_target_property(update ${plugin} FREI0R_PLUGIN_UPDATE)
    get_target_property(update2 ${plugin} FREI0R_PLUGIN_UPDATE2)
    get_target_property(compile_definitions ${plugin} FREI0R_PLUGIN_COMPILE_DEFINITIONS)
    get_target_property(compile_options ${plugin} FREI0R_PLUGIN_COMPILE_OPTIONS)
    get_target_property(link_libraries ${plugin} FREI0R_PLUGIN_LINK_LIBRARIES)
    set(object_target frei0r-bundle-object-${plugin})
    add_library(${object_target} OBJECT ${sources})
    if(compile_definitions)
      target_compile_definitions(${object_target} PRIVATE ${compile_definitions})
    endif()
    if(compile_options)
      target_compile_options(${object_target} PRIVATE ${compile_options})
    endif()
    frei0r_bundle_add_descriptor(
      TARGET ${object_target} ID ${id} UPDATE ${update} UPDATE2 ${update2}
      OUT_TARGET ${object_target}-descriptor COLLECTION production
    )
    set(descriptor_target ${object_target}-descriptor)
    set(symbol ${descriptor_target}_SYMBOL)
    set(symbol ${${symbol}})
    list(APPEND bundle_objects $<TARGET_OBJECTS:${object_target}>)
    list(APPEND bundle_descriptor_objects $<TARGET_OBJECTS:${descriptor_target}>)
    string(APPEND bundle_registry_declarations
      "extern const f0r_plugin_descriptor_t ${symbol};\n")
    string(APPEND bundle_registry_entries "  &${symbol},\n")
    list(APPEND bundle_link_libraries ${link_libraries})
  endforeach()

  set(FREI0R_BUNDLE_REGISTRY_DECLARATIONS "${bundle_registry_declarations}")
  set(FREI0R_BUNDLE_REGISTRY_ENTRIES "${bundle_registry_entries}")
  set(registry_source "${CMAKE_BINARY_DIR}/bundle-registry/frei0r-bundle-registry.c")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bundle-registry")
  configure_file("${FREI0R_BUNDLE_REGISTRY_TEMPLATE}"
                 "${registry_source}" @ONLY)
  add_library(frei0r-bundle-registry OBJECT "${registry_source}")
  target_compile_definitions(frei0r-bundle-registry PRIVATE FREI0R_BUNDLE_BUILD)
  set_target_properties(frei0r-bundle-registry PROPERTIES C_VISIBILITY_PRESET hidden)

  add_library(frei0r-bundle STATIC
    $<TARGET_OBJECTS:frei0r-bundle-registry>
    ${bundle_objects} ${bundle_descriptor_objects}
  )
  set_target_properties(frei0r-bundle PROPERTIES OUTPUT_NAME frei0r)
  target_include_directories(frei0r-bundle PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  )
  if(bundle_link_libraries)
    target_link_libraries(frei0r-bundle PRIVATE ${bundle_link_libraries})
  endif()

  if(BUILD_TESTING)
    add_library(frei0r-bundle-shared SHARED
      $<TARGET_OBJECTS:frei0r-bundle-registry>
      ${bundle_objects} ${bundle_descriptor_objects}
    )
    set_target_properties(frei0r-bundle-shared PROPERTIES OUTPUT_NAME frei0r-bundle-test)
    if(bundle_link_libraries)
      target_link_libraries(frei0r-bundle-shared PRIVATE ${bundle_link_libraries})
    endif()
  endif()
endfunction()
