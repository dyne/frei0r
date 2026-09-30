# Reusable descriptor generation for isolated frei0r bundle plugin objects.

include(CMakeParseArguments)

set(FREI0R_BUNDLE_PLUGIN_DESCRIPTOR_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/Frei0rBundlePluginDescriptor.c.in")
set(FREI0R_BUNDLE_REGISTRY_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/Frei0rBundleRegistry.c.in")
set(FREI0R_BUNDLE_SYMBOL_AUDIT_SCRIPT
    "${CMAKE_CURRENT_LIST_DIR}/Frei0rBundleSymbolAudit.cmake")

# Register a CTest audit for strong external implementation symbols emitted by
# the selected object targets.  A manifest is generated at build time so the
# audit follows the compiler's object-file layout without guessing paths.
function(frei0r_bundle_add_symbol_collision_audit)
  cmake_parse_arguments(ARG "" "NAME" "TARGETS" ${ARGN})
  if(NOT ARG_NAME OR NOT ARG_TARGETS)
    message(FATAL_ERROR
      "frei0r_bundle_add_symbol_collision_audit requires NAME and TARGETS")
  endif()
  if(NOT BUILD_TESTING)
    return()
  endif()

  set(manifests)
  foreach(target IN LISTS ARG_TARGETS)
    if(NOT TARGET ${target})
      message(FATAL_ERROR "symbol audit target '${target}' does not exist")
    endif()
    set(manifest "${CMAKE_CURRENT_BINARY_DIR}/symbol-audit/${ARG_NAME}-${target}.objects")
    file(GENERATE OUTPUT "${manifest}"
      CONTENT "$<JOIN:$<TARGET_OBJECTS:${target}>,\n>")
    list(APPEND manifests "${target}=${manifest}")
  endforeach()

  string(REPLACE ";" "," manifest_argument "${manifests}")
  add_custom_target(${ARG_NAME}-objects ALL DEPENDS ${ARG_TARGETS})
  add_test(NAME ${ARG_NAME}
    COMMAND ${CMAKE_COMMAND}
      -DNM=${CMAKE_NM}
      -DMANIFESTS=${manifest_argument}
      -P ${FREI0R_BUNDLE_SYMBOL_AUDIT_SCRIPT}
  )
endfunction()

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
    "NAME;KIND;BUNDLE_ID;BUNDLE_PROFILE;BUNDLE_REASON;UPDATE;UPDATE2;MSVC_DEFINITION"
    "SOURCES;COMPILE_DEFINITIONS;COMPILE_OPTIONS;LINK_LIBRARIES;BUNDLE_DEPENDENCIES" ${ARGN})
  foreach(required NAME KIND SOURCES)
    if(NOT ARG_${required})
      message(FATAL_ERROR "frei0r_add_plugin requires ${required}")
    endif()
  endforeach()
  if(NOT ARG_BUNDLE_ID)
    set(ARG_BUNDLE_ID "${ARG_NAME}")
  endif()
  if(NOT ARG_BUNDLE_PROFILE)
    set(ARG_BUNDLE_PROFILE core)
  endif()
  if(NOT ARG_BUNDLE_REASON)
    set(ARG_BUNDLE_REASON "language runtime and math library only")
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
    FREI0R_PLUGIN_BUNDLE_PROFILE "${ARG_BUNDLE_PROFILE}"
    FREI0R_PLUGIN_BUNDLE_REASON "${ARG_BUNDLE_REASON}"
    FREI0R_PLUGIN_BUNDLE_DEPENDENCIES "${ARG_BUNDLE_DEPENDENCIES}"
    FREI0R_PLUGIN_SOURCES "${bundle_sources}"
    FREI0R_PLUGIN_UPDATE "${ARG_UPDATE}"
    FREI0R_PLUGIN_UPDATE2 "${ARG_UPDATE2}"
    FREI0R_PLUGIN_COMPILE_DEFINITIONS "${ARG_COMPILE_DEFINITIONS}"
    FREI0R_PLUGIN_COMPILE_OPTIONS "${ARG_COMPILE_OPTIONS}"
    FREI0R_PLUGIN_LINK_LIBRARIES "${ARG_LINK_LIBRARIES}"
  )
  set_property(GLOBAL APPEND PROPERTY FREI0R_PLUGIN_TARGETS ${ARG_NAME})
  set_property(GLOBAL APPEND PROPERTY FREI0R_BUNDLE_CLASSIFICATIONS
    "${ARG_NAME}|${ARG_BUNDLE_PROFILE}|${ARG_BUNDLE_REASON}")
endfunction()

# Emscripten implements CMake MODULE targets as static libraries because Wasm
# has no native module ABI.  They are still plugin declarations and must enter
# the bundle classification/promotion pass just as MODULE targets do.
function(frei0r_bundle_is_plugin_target target output)
  get_target_property(type ${target} TYPE)
  set(is_plugin FALSE)
  if(type STREQUAL "MODULE_LIBRARY")
    set(is_plugin TRUE)
  elseif(EMSCRIPTEN AND CMAKE_CROSSCOMPILING AND type STREQUAL "STATIC_LIBRARY")
    set(is_plugin TRUE)
  endif()
  set(${output} ${is_plugin} PARENT_SCOPE)
endfunction()

# Legacy MODULE declarations are retained for ordinary frei0r deployment, but
# are still classified so profile resolution never silently omits a target.
function(frei0r_bundle_classify_legacy_modules directory)
  get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    frei0r_bundle_is_plugin_target(${target} is_plugin)
    if(NOT is_plugin)
      continue()
    endif()
    get_target_property(profile ${target} FREI0R_PLUGIN_BUNDLE_PROFILE)
    if(profile)
      continue()
    endif()
    set(profile unsupported)
    set(reason "legacy MODULE lacks bundle metadata and source-level collision audit")
    if(target STREQUAL "facebl0r" OR target STREQUAL "facedetect")
      set(profile optional-opencv)
      set(reason "requires OpenCV")
    elseif(target STREQUAL "cairoimagegrid" OR target STREQUAL "cairogradient" OR
           target STREQUAL "mirr0r" OR target STREQUAL "shake0scillate" OR
           target STREQUAL "cairoaffineblend" OR target STREQUAL "cairoblend")
      set(profile optional-cairo)
      set(reason "requires Cairo")
    elseif(target STREQUAL "rgbparade" OR target STREQUAL "scale0tilt" OR
           target STREQUAL "vectorscope")
      set(profile optional-gavl)
      set(reason "requires GAVL")
    elseif(target STREQUAL "shadert0y")
      set(profile optional-opengl-egl)
      set(reason "requires OpenGL/EGL platform support")
    elseif(target STREQUAL "colgate" OR target STREQUAL "ndvi")
      set(profile unsupported-dynamic-loading)
      set(reason "uses dynamic loading and is not portable to static hosts")
    endif()
    set_target_properties(${target} PROPERTIES
      FREI0R_PLUGIN_BUNDLE_PROFILE "${profile}"
      FREI0R_PLUGIN_BUNDLE_REASON "${reason}"
      FREI0R_PLUGIN_BUNDLE_ELIGIBLE FALSE)
    set_property(GLOBAL APPEND PROPERTY FREI0R_BUNDLE_CLASSIFICATIONS
      "${target}|${profile}|${reason}")
  endforeach()
  get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    frei0r_bundle_classify_legacy_modules("${subdirectory}")
  endforeach()
endfunction()

# Promote classified portable MODULE targets into the same metadata contract as
# explicitly declared plugins.  Their source paths and kind are properties of
# the ordinary target, so this keeps MODULE behavior unchanged.
function(frei0r_bundle_promote_core_modules directory)
  get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    frei0r_bundle_is_plugin_target(${target} is_plugin)
    get_target_property(profile ${target} FREI0R_PLUGIN_BUNDLE_PROFILE)
    get_target_property(reason ${target} FREI0R_PLUGIN_BUNDLE_REASON)
    get_target_property(eligible ${target} FREI0R_PLUGIN_BUNDLE_ELIGIBLE)
    if(NOT is_plugin OR eligible OR
       profile STREQUAL "unsupported-dynamic-loading")
      continue()
    endif()
    get_target_property(sources ${target} SOURCES)
    get_target_property(source_dir ${target} SOURCE_DIR)
    set(absolute_sources)
    foreach(source IN LISTS sources)
      if(IS_ABSOLUTE "${source}")
        list(APPEND absolute_sources "${source}")
      else()
        list(APPEND absolute_sources "${source_dir}/${source}")
      endif()
    endforeach()
    set(update FALSE)
    set(update2 FALSE)
    foreach(source IN LISTS absolute_sources)
      file(READ "${source}" source_contents)
      if(source_contents MATCHES "f0r_update[ \\t\\r\\n]*\\(")
        set(update TRUE)
      endif()
      if(source_contents MATCHES "f0r_update2[ \\t\\r\\n]*\\(")
        set(update2 TRUE)
      endif()
    endforeach()
    if(source_dir MATCHES "/generator/")
      set(kind SOURCE)
    elseif(source_dir MATCHES "/mixer3/")
      set(kind MIXER3)
    elseif(source_dir MATCHES "/mixer2/")
      set(kind MIXER2)
    else()
      set(kind FILTER)
    endif()
    if(kind STREQUAL "FILTER" AND NOT update AND NOT update2)
      set(update TRUE)
    elseif(kind STREQUAL "SOURCE" AND NOT update AND NOT update2)
      set(update TRUE)
    elseif(kind STREQUAL "MIXER2" AND NOT update AND NOT update2)
      set(update TRUE)
      set(update2 TRUE)
    elseif(kind STREQUAL "MIXER3" AND NOT update AND NOT update2)
      set(update2 TRUE)
    endif()
    set(dependencies "")
    if(profile STREQUAL "unsupported")
      set(profile core)
      set(reason "language runtime and math library only")
    elseif(profile STREQUAL "optional-opencv")
      set(dependencies OpenCV_FOUND)
    elseif(profile STREQUAL "optional-cairo")
      set(dependencies Cairo_FOUND)
    elseif(profile STREQUAL "optional-gavl")
      set(dependencies GAVL_FOUND)
    elseif(profile STREQUAL "optional-opengl-egl")
      set(dependencies OPENGL_TARGET EGL_FOUND)
    else()
      continue()
    endif()
    get_target_property(include_directories ${target} INCLUDE_DIRECTORIES)
    get_target_property(link_libraries ${target} LINK_LIBRARIES)
    get_target_property(compile_definitions ${target} COMPILE_DEFINITIONS)
    get_target_property(compile_options ${target} COMPILE_OPTIONS)
    set_target_properties(${target} PROPERTIES
      FREI0R_PLUGIN_KIND "${kind}"
      FREI0R_PLUGIN_BUNDLE_ID "${target}"
      FREI0R_PLUGIN_BUNDLE_ELIGIBLE TRUE
      FREI0R_PLUGIN_BUNDLE_PROFILE "${profile}"
      FREI0R_PLUGIN_BUNDLE_REASON "${reason}"
      FREI0R_PLUGIN_BUNDLE_DEPENDENCIES "${dependencies}"
      FREI0R_PLUGIN_SOURCES "${absolute_sources}"
      FREI0R_PLUGIN_UPDATE "${update}"
      FREI0R_PLUGIN_UPDATE2 "${update2}"
      FREI0R_PLUGIN_COMPILE_DEFINITIONS "${compile_definitions}"
      FREI0R_PLUGIN_COMPILE_OPTIONS "${compile_options}"
      FREI0R_PLUGIN_INCLUDE_DIRECTORIES "${include_directories}"
      FREI0R_PLUGIN_LINK_LIBRARIES "${link_libraries}")
    set_property(GLOBAL APPEND PROPERTY FREI0R_PLUGIN_TARGETS ${target})
  endforeach()
  get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    frei0r_bundle_promote_core_modules("${subdirectory}")
  endforeach()
endfunction()

# Persist classifications only after promotion has established the final
# metadata.  This prevents a stale pre-promotion "unsupported" record from
# misrepresenting an eligible core or optional bundle target.
function(frei0r_bundle_collect_module_targets directory output)
  get_property(directory_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  set(targets)
  foreach(target IN LISTS directory_targets)
    frei0r_bundle_is_plugin_target(${target} is_plugin)
    if(is_plugin)
      list(APPEND targets ${target})
    endif()
  endforeach()
  get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    frei0r_bundle_collect_module_targets("${subdirectory}" child_targets)
    list(APPEND targets ${child_targets})
  endforeach()
  set(${output} "${targets}" PARENT_SCOPE)
endfunction()

function(frei0r_bundle_collect_classifications directory)
  get_property(targets GLOBAL PROPERTY FREI0R_PLUGIN_TARGETS)
  frei0r_bundle_collect_module_targets("${directory}" module_targets)
  list(APPEND targets ${module_targets})
  list(REMOVE_DUPLICATES targets)
  set(FREI0R_BUNDLE_CLASSIFICATION_TARGETS "${targets}" CACHE INTERNAL
      "Targets considered by the final bundle classification" FORCE)
  foreach(target IN LISTS targets)
    get_target_property(profile ${target} FREI0R_PLUGIN_BUNDLE_PROFILE)
    get_target_property(reason ${target} FREI0R_PLUGIN_BUNDLE_REASON)
    if(NOT profile OR profile MATCHES "-NOTFOUND$")
      if(target STREQUAL "facebl0r" OR target STREQUAL "facedetect")
        set(profile optional-opencv)
        set(reason "requires OpenCV")
      elseif(target STREQUAL "cairoimagegrid" OR target STREQUAL "cairogradient" OR
             target STREQUAL "mirr0r" OR target STREQUAL "shake0scillate" OR
             target STREQUAL "cairoaffineblend" OR target STREQUAL "cairoblend")
        set(profile optional-cairo)
        set(reason "requires Cairo")
      elseif(target STREQUAL "rgbparade" OR target STREQUAL "scale0tilt" OR
             target STREQUAL "vectorscope")
        set(profile optional-gavl)
        set(reason "requires GAVL")
      elseif(target STREQUAL "shadert0y")
        set(profile optional-opengl-egl)
        set(reason "requires OpenGL/EGL platform support")
      elseif(target STREQUAL "colgate" OR target STREQUAL "ndvi")
        set(profile unsupported-dynamic-loading)
        set(reason "uses dynamic loading and is not portable to static hosts")
      endif()
    endif()
    if(NOT "${profile}" STREQUAL "" AND NOT "${profile}" MATCHES "-NOTFOUND$" AND
       NOT "${reason}" STREQUAL "" AND NOT "${reason}" MATCHES "-NOTFOUND$")
      set_property(GLOBAL APPEND PROPERTY FREI0R_BUNDLE_CLASSIFICATIONS
        "${target}|${profile}|${reason}")
    endif()
  endforeach()
endfunction()

function(frei0r_bundle_dependencies_available output)
  set(available TRUE)
  foreach(dependency IN LISTS ARGN)
    if(dependency STREQUAL "OPENGL_TARGET")
      if(NOT TARGET OpenGL::GL)
        set(available FALSE)
      endif()
    elseif(NOT ${dependency})
      set(available FALSE)
    endif()
  endforeach()
  set(${output} ${available} PARENT_SCOPE)
endfunction()

function(frei0r_bundle_resolve_targets output)
  get_property(plugin_targets GLOBAL PROPERTY FREI0R_PLUGIN_TARGETS)
  if(FREI0R_BUNDLE_PLUGINS)
    set(requested ${FREI0R_BUNDLE_PLUGINS})
  elseif(FREI0R_BUNDLE_PROFILE STREQUAL "core")
    set(requested)
    foreach(plugin IN LISTS plugin_targets)
      get_target_property(profile ${plugin} FREI0R_PLUGIN_BUNDLE_PROFILE)
      if(profile STREQUAL "core")
        list(APPEND requested ${plugin})
      endif()
    endforeach()
  elseif(FREI0R_BUNDLE_PROFILE STREQUAL "all")
    set(requested)
    foreach(plugin IN LISTS plugin_targets)
      get_target_property(dependencies ${plugin} FREI0R_PLUGIN_BUNDLE_DEPENDENCIES)
      frei0r_bundle_dependencies_available(available ${dependencies})
      if(available)
        list(APPEND requested ${plugin})
      endif()
    endforeach()
  else()
    message(FATAL_ERROR
      "FREI0R_BUNDLE_PROFILE must be core or all, got '${FREI0R_BUNDLE_PROFILE}'")
  endif()

  if(NOT requested)
    message(FATAL_ERROR "frei0r bundle profile '${FREI0R_BUNDLE_PROFILE}' resolved no targets")
  endif()
  list(REMOVE_DUPLICATES requested)
  foreach(plugin IN LISTS requested)
    if(NOT TARGET ${plugin})
      message(FATAL_ERROR "FREI0R_BUNDLE_PLUGINS selects unknown plugin '${plugin}'")
    endif()
    get_target_property(eligible ${plugin} FREI0R_PLUGIN_BUNDLE_ELIGIBLE)
    if(NOT eligible)
      message(FATAL_ERROR "plugin '${plugin}' is not bundle eligible")
    endif()
    get_target_property(dependencies ${plugin} FREI0R_PLUGIN_BUNDLE_DEPENDENCIES)
    frei0r_bundle_dependencies_available(available ${dependencies})
    if(NOT available)
      message(FATAL_ERROR
        "plugin '${plugin}' requires unavailable bundle dependency '${dependencies}'")
    endif()
  endforeach()
  set(${output} "${requested}" PARENT_SCOPE)
endfunction()

function(frei0r_finalize_bundle)
  if(NOT FREI0R_BUILD_BUNDLE)
    return()
  endif()
  frei0r_bundle_resolve_targets(resolved_plugins)
  set(FREI0R_BUNDLE_RESOLVED_TARGETS "${resolved_plugins}" CACHE INTERNAL
      "Targets selected for the current frei0r bundle" FORCE)

  set(bundle_objects)
  set(bundle_object_targets)
  set(bundle_descriptor_objects)
  set(bundle_registry_declarations)
  set(bundle_registry_entries)
  set(bundle_link_libraries)
  set(bundle_install_link_libraries)
  set(bundle_needs_opencv OFF)
  set(bundle_needs_cairo OFF)
  set(bundle_needs_gavl OFF)
  set(bundle_needs_opengl_egl OFF)
  foreach(plugin IN LISTS resolved_plugins)
    get_target_property(sources ${plugin} FREI0R_PLUGIN_SOURCES)
    get_target_property(id ${plugin} FREI0R_PLUGIN_BUNDLE_ID)
    get_target_property(update ${plugin} FREI0R_PLUGIN_UPDATE)
    get_target_property(update2 ${plugin} FREI0R_PLUGIN_UPDATE2)
    get_target_property(compile_definitions ${plugin} FREI0R_PLUGIN_COMPILE_DEFINITIONS)
    get_target_property(compile_options ${plugin} FREI0R_PLUGIN_COMPILE_OPTIONS)
    get_target_property(include_directories ${plugin} FREI0R_PLUGIN_INCLUDE_DIRECTORIES)
    get_target_property(link_libraries ${plugin} FREI0R_PLUGIN_LINK_LIBRARIES)
    get_target_property(profile ${plugin} FREI0R_PLUGIN_BUNDLE_PROFILE)
    if(profile STREQUAL "optional-opencv")
      set(bundle_needs_opencv ON)
    elseif(profile STREQUAL "optional-cairo")
      set(bundle_needs_cairo ON)
    elseif(profile STREQUAL "optional-gavl")
      set(bundle_needs_gavl ON)
    elseif(profile STREQUAL "optional-opengl-egl")
      set(bundle_needs_opengl_egl ON)
    endif()
    if(FREI0R_BUNDLE_WASM_PTHREADS)
      list(REMOVE_ITEM compile_definitions NO_FUTURE)
    endif()
    if(FREI0R_BUNDLE_WASM_BASELINE OR FREI0R_BUNDLE_WASM_SIMD OR
       FREI0R_BUNDLE_WASM_PTHREADS)
      set(scalar_compile_options)
      foreach(option IN LISTS compile_options)
        if(option MATCHES "^-m(sse|avx)" OR option STREQUAL "-pthread")
          continue()
        endif()
        list(APPEND scalar_compile_options "${option}")
      endforeach()
      set(compile_options ${scalar_compile_options})
      set(scalar_link_libraries)
      foreach(library IN LISTS link_libraries)
        if((library STREQUAL "Threads::Threads" OR library MATCHES "pthread") AND
           NOT FREI0R_BUNDLE_WASM_PTHREADS)
          continue()
        endif()
        list(APPEND scalar_link_libraries "${library}")
      endforeach()
      set(link_libraries ${scalar_link_libraries})
    endif()
    set(object_target frei0r-bundle-object-${plugin})
    add_library(${object_target} OBJECT ${sources})
    set_target_properties(${object_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(FREI0R_BUNDLE_WASM_BASELINE OR FREI0R_BUNDLE_WASM_SIMD OR
       FREI0R_BUNDLE_WASM_PTHREADS)
      # These two plugins have existing serial code paths; do not let the
      # Wasm archive acquire x86 intrinsics.
      if(plugin STREQUAL "kaleid0sc0pe")
        target_compile_definitions(${object_target} PRIVATE NO_SSE2)
      endif()
      target_compile_options(${object_target} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>")
      if(NOT FREI0R_BUNDLE_WASM_PTHREADS AND
         (plugin STREQUAL "kaleid0sc0pe" OR plugin STREQUAL "squigglevision"))
        target_compile_definitions(${object_target} PRIVATE NO_FUTURE)
      endif()
    endif()
    if(FREI0R_BUNDLE_WASM_SIMD)
      target_compile_options(${object_target} PRIVATE -msimd128)
      # These stochastic effects are sensitive to vectorized floating-point
      # reduction order.  Keep their scalar loops so the optional profile has
      # the same deterministic frame contract as the baseline.
      if(plugin STREQUAL "cluster" OR plugin STREQUAL "filmgrain" OR
         plugin STREQUAL "rgbnoise" OR plugin STREQUAL "partik0l")
        target_compile_options(${object_target} PRIVATE
          -fno-vectorize -fno-slp-vectorize)
      endif()
    elseif(FREI0R_BUNDLE_WASM_PTHREADS)
      target_compile_options(${object_target} PRIVATE -pthread)
    endif()
    if(include_directories)
      target_include_directories(${object_target} PRIVATE ${include_directories})
    endif()
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
    list(APPEND bundle_object_targets ${object_target})
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
  list(REMOVE_DUPLICATES bundle_link_libraries)
  # Preserve only portable system names directly in the installed interface.
  # Optional dependencies are rediscovered by the installed package and use
  # targets created there, so its export never captures this build host's
  # library paths or package-specific target names.
  foreach(library IN LISTS bundle_link_libraries)
    if(library STREQUAL "m" OR library STREQUAL "-lm")
      list(APPEND bundle_install_link_libraries "${library}")
    endif()
  endforeach()
  list(FIND bundle_link_libraries "Threads::Threads" bundle_threads_index)
  if(bundle_threads_index EQUAL -1)
    set(FREI0R_BUNDLE_NEEDS_THREADS OFF CACHE INTERNAL
        "Whether the selected bundle exports a Threads dependency" FORCE)
  else()
    set(FREI0R_BUNDLE_NEEDS_THREADS ON CACHE INTERNAL
        "Whether the selected bundle exports a Threads dependency" FORCE)
    list(APPEND bundle_install_link_libraries Threads::Threads)
  endif()
  if(bundle_needs_gavl)
    list(APPEND bundle_install_link_libraries PkgConfig::Frei0rBundleGavl)
  endif()
  if(bundle_needs_opencv)
    list(APPEND bundle_install_link_libraries Frei0rBundle::opencv)
  endif()
  if(bundle_needs_cairo)
    list(APPEND bundle_install_link_libraries PkgConfig::Frei0rBundleCairo)
  endif()
  if(bundle_needs_opengl_egl)
    list(APPEND bundle_install_link_libraries OpenGL::GL PkgConfig::Frei0rBundleEGL)
  endif()
  set(FREI0R_BUNDLE_NEEDS_OPENCV ${bundle_needs_opencv} CACHE INTERNAL
      "Whether the selected bundle exports an OpenCV dependency" FORCE)
  set(FREI0R_BUNDLE_NEEDS_CAIRO ${bundle_needs_cairo} CACHE INTERNAL
      "Whether the selected bundle exports a Cairo dependency" FORCE)
  set(FREI0R_BUNDLE_NEEDS_GAVL ${bundle_needs_gavl} CACHE INTERNAL
      "Whether the selected bundle exports a GAVL dependency" FORCE)
  set(FREI0R_BUNDLE_NEEDS_OPENGL_EGL ${bundle_needs_opengl_egl} CACHE INTERNAL
      "Whether the selected bundle exports OpenGL/EGL dependencies" FORCE)
  set_target_properties(frei0r-bundle PROPERTIES
    OUTPUT_NAME frei0r-bundle-static
    EXPORT_NAME static
  )
  target_include_directories(frei0r-bundle PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  if(bundle_link_libraries)
    # Static archives do not carry their dependent libraries.  Preserve the
    # source build's exact resolved order, but make installed consumers use
    # dependencies rediscovered by Frei0rBundleConfig.cmake.
    target_link_libraries(frei0r-bundle PUBLIC
      "$<BUILD_INTERFACE:${bundle_link_libraries}>"
      "$<INSTALL_INTERFACE:${bundle_install_link_libraries}>"
    )
  endif()

  if(NOT _frei0r_wasm_target OR BUILD_TESTING)
    add_library(frei0r-bundle-shared SHARED
      $<TARGET_OBJECTS:frei0r-bundle-registry>
      ${bundle_objects} ${bundle_descriptor_objects}
    )
    set_target_properties(frei0r-bundle-shared PROPERTIES
      OUTPUT_NAME frei0r-bundle
      EXPORT_NAME shared
      C_VISIBILITY_PRESET hidden
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN YES
    )
    if(UNIX AND NOT APPLE)
      set(bundle_exports "${CMAKE_CURRENT_BINARY_DIR}/frei0r-bundle.exports")
      file(WRITE "${bundle_exports}" "{ global: f0r_bundle_plugin_count; f0r_bundle_plugin_by_index; f0r_bundle_plugin_by_id; local: *; };\n")
      target_link_options(frei0r-bundle-shared PRIVATE
        "-Wl,--version-script=${bundle_exports}")
    endif()
    if(bundle_link_libraries)
      target_link_libraries(frei0r-bundle-shared PRIVATE ${bundle_link_libraries})
    endif()

    if(NOT _frei0r_wasm_target)
      install(TARGETS frei0r-bundle frei0r-bundle-shared
        EXPORT Frei0rBundleTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )
    endif()
  endif()

  if(BUILD_TESTING)
    frei0r_bundle_add_symbol_collision_audit(
      NAME frei0r-bundle-symbol-collisions TARGETS ${bundle_object_targets}
    )
  endif()
endfunction()
