option(TEST_ASAN "Run tests in asan mode" OFF)
option(TEST_GUI "Run tests in gui mode" OFF)

include(${CMAKE_SOURCE_DIR}/cmake/Frei0rBundlePlugin.cmake)

add_executable(frei0r-run test-pattern.c frei0r-run.c)
add_executable(frei0r-meta frei0r-meta.c)

# This intentionally compiles plugins with private entry point names so that
# the archive/registry path can be exercised without changing MODULE targets.
add_library(runtime-smoke-brightness OBJECT
  ${CMAKE_SOURCE_DIR}/src/filter/brightness/brightness.c
)
frei0r_bundle_add_descriptor(
  TARGET runtime-smoke-brightness ID brightness UPDATE TRUE UPDATE2 FALSE
  OUT_TARGET runtime-smoke-brightness-descriptor
)

add_library(runtime-smoke-invert0r OBJECT
  ${CMAKE_SOURCE_DIR}/src/filter/invert0r/invert0r.c
)
frei0r_bundle_add_descriptor(
  TARGET runtime-smoke-invert0r ID invert0r UPDATE TRUE UPDATE2 FALSE
  OUT_TARGET runtime-smoke-invert0r-descriptor
)

add_library(runtime-smoke-plugins STATIC
  $<TARGET_OBJECTS:runtime-smoke-brightness>
  $<TARGET_OBJECTS:runtime-smoke-invert0r>
  $<TARGET_OBJECTS:runtime-smoke-brightness-descriptor>
  $<TARGET_OBJECTS:runtime-smoke-invert0r-descriptor>
)

add_library(bundle-prototype-addition OBJECT
  ${CMAKE_SOURCE_DIR}/src/mixer2/addition/addition.cpp
)
frei0r_bundle_add_descriptor(
  TARGET bundle-prototype-addition ID addition UPDATE TRUE UPDATE2 TRUE
  OUT_TARGET bundle-prototype-addition-descriptor
)

add_library(bundle-prototype-plugins STATIC
  $<TARGET_OBJECTS:runtime-smoke-brightness>
  $<TARGET_OBJECTS:runtime-smoke-brightness-descriptor>
  $<TARGET_OBJECTS:bundle-prototype-addition>
  $<TARGET_OBJECTS:bundle-prototype-addition-descriptor>
)

frei0r_bundle_add_symbol_collision_audit(
  NAME bundle-prototype-symbol-collisions
  TARGETS runtime-smoke-brightness runtime-smoke-invert0r bundle-prototype-addition
)

add_library(symbol-collision-fixture-a OBJECT symbol-collision-fixture-a.c)
add_library(symbol-collision-fixture-b OBJECT symbol-collision-fixture-b.c)
add_custom_target(bundle-symbol-collision-fixture-objects ALL
  DEPENDS symbol-collision-fixture-a symbol-collision-fixture-b)
set(symbol_collision_fixture_manifests
  "symbol-collision-fixture-a=${CMAKE_CURRENT_BINARY_DIR}/symbol-audit/fixture-a.objects"
  "symbol-collision-fixture-b=${CMAKE_CURRENT_BINARY_DIR}/symbol-audit/fixture-b.objects")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/symbol-audit/fixture-a.objects"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:symbol-collision-fixture-a>,\n>")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/symbol-audit/fixture-b.objects"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:symbol-collision-fixture-b>,\n>")
string(REPLACE ";" "," symbol_collision_fixture_manifest_argument
  "${symbol_collision_fixture_manifests}")
add_test(NAME bundle-symbol-collision-fixture
  COMMAND ${CMAKE_COMMAND}
    -DAUDIT_SCRIPT=${CMAKE_SOURCE_DIR}/cmake/Frei0rBundleSymbolAudit.cmake
    -DNM=${CMAKE_NM}
    -DMANIFESTS=${symbol_collision_fixture_manifest_argument}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-symbol-collision-fixture.cmake
)

add_library(runtime-smoke-registry OBJECT runtime-smoke-registry.c)

add_library(bundle-prototype-registry OBJECT bundle-prototype-registry.c)
target_compile_definitions(bundle-prototype-registry PRIVATE FREI0R_BUNDLE_BUILD)

add_executable(bundle-prototype-contract bundle-prototype-contract.cpp
  $<TARGET_OBJECTS:bundle-prototype-registry>
)
target_link_libraries(bundle-prototype-contract PRIVATE bundle-prototype-plugins)
add_test(NAME bundle-prototype-contract COMMAND bundle-prototype-contract)

add_library(bundle-symbol-probe SHARED
  $<TARGET_OBJECTS:bundle-prototype-registry>
  $<TARGET_OBJECTS:runtime-smoke-brightness>
  $<TARGET_OBJECTS:runtime-smoke-brightness-descriptor>
  $<TARGET_OBJECTS:bundle-prototype-addition>
  $<TARGET_OBJECTS:bundle-prototype-addition-descriptor>
)
set_target_properties(bundle-symbol-probe PROPERTIES
  C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
add_test(NAME bundle-symbol-probe-exports
  COMMAND ${CMAKE_COMMAND}
    -DPROBE=$<TARGET_FILE:bundle-symbol-probe>
    -DNM=${CMAKE_NM}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-symbols.cmake
)
add_test(NAME ordinary-module-symbols
  COMMAND ${CMAKE_COMMAND}
    -DPROBE=$<TARGET_FILE:brightness>
    -DNM=${CMAKE_NM}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-ordinary-module-symbols.cmake
)
add_test(NAME bundle-id-collision
  COMMAND ${CMAKE_COMMAND}
    -S ${CMAKE_CURRENT_SOURCE_DIR}/bundle-id-collision
    -B ${CMAKE_CURRENT_BINARY_DIR}/bundle-id-collision
)
set_tests_properties(bundle-id-collision PROPERTIES WILL_FAIL TRUE)

add_test(NAME bundle-generated-registry-duplicates
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/bundle-generated-registry-duplicates
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-generated-registry-duplicates.cmake
)

add_test(NAME bundle-default-off
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/bundle-default-off
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-default-off.cmake
)

foreach(profile core all)
  set(profile_arguments
    -DEXPECT_CLASSIFICATION_COUNT=159
    "-DEXPECT_CLASSIFICATION_RECORDS=brightness|core|language runtime and math library only,colgate|unsupported-dynamic-loading|uses dynamic loading and is not portable to static hosts")
  if(profile STREQUAL "all")
    list(APPEND profile_arguments -DNATIVE_DEPS=ON)
    list(REMOVE_ITEM profile_arguments -DEXPECT_CLASSIFICATION_COUNT=159)
    list(APPEND profile_arguments -DEXPECT_CLASSIFICATION_COUNT=170)
    list(REMOVE_ITEM profile_arguments "-DEXPECT_CLASSIFICATION_RECORDS=brightness|core|language runtime and math library only,colgate|unsupported-dynamic-loading|uses dynamic loading and is not portable to static hosts")
    list(APPEND profile_arguments
      "-DEXPECT_CLASSIFICATION_RECORDS=brightness|core|language runtime and math library only,cairoimagegrid|optional-cairo|requires Cairo,colgate|unsupported-dynamic-loading|uses dynamic loading and is not portable to static hosts")
    list(APPEND profile_arguments -DEXPECT_TARGETS=cairoimagegrid,rgbparade)
  endif()
  add_test(NAME bundle-profile-${profile}
    COMMAND ${CMAKE_COMMAND}
      -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/bundle-profile-${profile}
      -DPROFILE=${profile}
      ${profile_arguments}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-profile.cmake
  )
endforeach()
add_test(NAME bundle-profile-explicit
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/bundle-profile-explicit
    -DPROFILE=core
    -DPLUGINS=brightness,cairoimagegrid
    -DNATIVE_DEPS=ON
    -DEXPECT_TARGETS=brightness,cairoimagegrid
    -DEXPECT_CLASSIFICATION_COUNT=170
    "-DEXPECT_CLASSIFICATION_RECORDS=brightness|core|language runtime and math library only,cairoimagegrid|optional-cairo|requires Cairo,colgate|unsupported-dynamic-loading|uses dynamic loading and is not portable to static hosts"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-profile.cmake
)
add_test(NAME bundle-profile-missing-dependency
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/bundle-profile-missing-dependency
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-profile-missing-dependency.cmake
)

if(TARGET frei0r-bundle)
  add_executable(bundle-native-consumer bundle-native-consumer.c)
  target_link_libraries(bundle-native-consumer PRIVATE frei0r-bundle)
  add_test(NAME bundle-native-consumer COMMAND bundle-native-consumer)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_executable(bundle-native-consumer-gc bundle-native-consumer.c)
    target_compile_options(bundle-native-consumer-gc PRIVATE
      -ffunction-sections -fdata-sections)
    target_link_options(bundle-native-consumer-gc PRIVATE -Wl,--gc-sections)
    target_link_libraries(bundle-native-consumer-gc PRIVATE frei0r-bundle)
    add_test(NAME bundle-native-consumer-gc COMMAND bundle-native-consumer-gc)
  endif()

  add_executable(bundle-registry-contract bundle-registry-contract.c)
  target_link_libraries(bundle-registry-contract PRIVATE frei0r-bundle)
  add_test(NAME bundle-registry-contract COMMAND bundle-registry-contract)

  add_test(NAME frei0r-bundle-shared-exports
    COMMAND ${CMAKE_COMMAND}
      -DPROBE=$<TARGET_FILE:frei0r-bundle-shared>
      -DNM=${CMAKE_NM}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-symbols.cmake
  )

  if(NOT _frei0r_wasm_target)
    add_test(NAME bundle-install-consumers
      COMMAND ${CMAKE_COMMAND}
        -DBUNDLE_BUILD_DIR=${CMAKE_BINARY_DIR}
        -DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/bundle-install-consumer
        -DPREFIX=${CMAKE_CURRENT_BINARY_DIR}/bundle-install-prefix
        -DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
        -DSTATIC_LIBRARY_PREFIX=${CMAKE_STATIC_LIBRARY_PREFIX}
        -DSTATIC_LIBRARY_SUFFIX=${CMAKE_STATIC_LIBRARY_SUFFIX}
        -DSHARED_LIBRARY_PREFIX=${CMAKE_SHARED_LIBRARY_PREFIX}
        -DSHARED_LIBRARY_SUFFIX=${CMAKE_SHARED_LIBRARY_SUFFIX}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-install.cmake
    )
  endif()
endif()

add_executable(bundle-api-contract bundle-api-contract.c
  $<TARGET_OBJECTS:runtime-smoke-registry>
)
target_link_libraries(bundle-api-contract PRIVATE runtime-smoke-plugins)
add_test(NAME bundle-api-contract COMMAND bundle-api-contract)

add_executable(bundle-api-cxx bundle-api-cxx.cpp)
add_test(NAME bundle-api-cxx COMMAND bundle-api-cxx)

if(UNIX)
  find_program(FREI0R_BASH_EXECUTABLE bash)
  find_program(FREI0R_SHA256SUM_EXECUTABLE sha256sum)
  find_program(FREI0R_TAR_EXECUTABLE tar)
  if(FREI0R_BASH_EXECUTABLE AND FREI0R_SHA256SUM_EXECUTABLE AND
     FREI0R_TAR_EXECUTABLE)
    add_test(NAME wasm-release-package
      COMMAND ${FREI0R_BASH_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/check-wasm-release-package.sh
        ${CMAKE_SOURCE_DIR}/.github/scripts/package-wasm-release.sh
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  option(FREI0R_RUNTIME_SMOKE_GC "Enable section garbage collection for the WASI smoke module" OFF)
  option(FREI0R_BUNDLE_WASI_GC "Enable section garbage collection for the WASI bundle runner" OFF)
  add_executable(runtime-smoke-wasi runtime-smoke.c
    $<TARGET_OBJECTS:runtime-smoke-registry>
  )
  target_compile_definitions(runtime-smoke-wasi PRIVATE RUNTIME_SMOKE_NO_MAIN)
  target_link_libraries(runtime-smoke-wasi PRIVATE runtime-smoke-plugins)
  set_target_properties(runtime-smoke-wasi PROPERTIES SUFFIX ".wasm")
  set_property(TARGET runtime-smoke-wasi APPEND_STRING PROPERTY LINK_FLAGS
    " -nostartfiles -Wl,--no-entry -Wl,--export=runtime_smoke_run")
  if(FREI0R_RUNTIME_SMOKE_GC)
    set_property(TARGET runtime-smoke-wasi APPEND_STRING PROPERTY LINK_FLAGS
      " -Wl,--gc-sections")
  endif()
  add_executable(measure-pr0file-wasi
    measure-pr0file-wasi.c
    ${CMAKE_SOURCE_DIR}/src/filter/measure/measure_pr0file.c)
  set_target_properties(measure-pr0file-wasi PROPERTIES SUFFIX ".wasm")
  set_property(TARGET measure-pr0file-wasi APPEND_STRING PROPERTY LINK_FLAGS
    " -nostartfiles -Wl,--no-entry -Wl,--initial-memory=8388608 -Wl,--export=measure_pr0file_wasi_run")
  find_program(FREI0R_NODE_EXECUTABLE node)
  if(FREI0R_NODE_EXECUTABLE)
    add_test(NAME bundle-registry-wasi
      COMMAND ${FREI0R_NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run-bundle-registry-wasi.mjs
        $<TARGET_FILE:bundle-registry-wasi>)
    add_test(NAME bundle-wasi-consumer
      COMMAND ${FREI0R_NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run-wasi-command.mjs
        $<TARGET_FILE:bundle-wasi-consumer>)
    add_test(NAME measure-pr0file-wasi
      COMMAND ${FREI0R_NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run-wasi-function.mjs
        $<TARGET_FILE:measure-pr0file-wasi> measure_pr0file_wasi_run)
  endif()
  add_executable(bundle-registry-wasi bundle-registry-wasi.c bundle-registry-contract.c)
  target_compile_definitions(bundle-registry-wasi PRIVATE BUNDLE_REGISTRY_CONTRACT_NO_MAIN)
  target_link_libraries(bundle-registry-wasi PRIVATE frei0r-bundle c++ c++abi)
  set_target_properties(bundle-registry-wasi PROPERTIES SUFFIX ".wasm")
  set_property(TARGET bundle-registry-wasi APPEND_STRING PROPERTY LINK_FLAGS
    " -mexec-model=reactor -Wl,--initial-memory=8388608 -Wl,--export-memory -Wl,--export=f0r_bundle_wasi_run -Wl,--export=f0r_bundle_wasi_output_digest -Wl,--export=f0r_bundle_wasi_application_frame_count -Wl,--export=f0r_bundle_wasi_parameter_change_count -Wl,--export=f0r_bundle_wasi_failure_id -Wl,--export=f0r_bundle_wasi_failure_stage")
  if(FREI0R_BUNDLE_WASI_GC)
    set_property(TARGET bundle-registry-wasi APPEND_STRING PROPERTY LINK_FLAGS
      " -Wl,--gc-sections")
  endif()
  add_executable(bundle-wasi-consumer bundle-native-consumer.c)
  target_link_libraries(bundle-wasi-consumer PRIVATE frei0r-bundle)
  set_target_properties(bundle-wasi-consumer PROPERTIES SUFFIX ".wasm")
elseif(EMSCRIPTEN)
  option(FREI0R_BUNDLE_EMSCRIPTEN_GC
    "Enable section garbage collection for the Emscripten bundle runner" OFF)
  set(FREI0R_BUNDLE_EMSCRIPTEN_SCALAR_REFERENCE "" CACHE FILEPATH
    "Scalar Emscripten registry module used to compare an experimental profile")
  add_executable(runtime-smoke-emscripten runtime-smoke.c
    $<TARGET_OBJECTS:runtime-smoke-registry>
  )
  target_compile_definitions(runtime-smoke-emscripten PRIVATE RUNTIME_SMOKE_NO_MAIN)
  target_link_libraries(runtime-smoke-emscripten PRIVATE runtime-smoke-plugins)
  set_property(TARGET runtime-smoke-emscripten APPEND_STRING PROPERTY LINK_FLAGS
    " --no-entry -sMODULARIZE=1 -sEXPORT_NAME=createRuntimeSmoke -sENVIRONMENT=node -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS=['_runtime_smoke_run']")
  add_executable(bundle-registry-emscripten
    bundle-registry-emscripten.c bundle-registry-contract.c)
  target_compile_definitions(bundle-registry-emscripten PRIVATE
    BUNDLE_REGISTRY_CONTRACT_NO_MAIN)
  target_link_libraries(bundle-registry-emscripten PRIVATE frei0r-bundle)
  set_property(TARGET bundle-registry-emscripten APPEND_STRING PROPERTY LINK_FLAGS
    " --no-entry -sINITIAL_MEMORY=67108864 -sMODULARIZE=1 -sEXPORT_NAME=createBundleRegistry -sENVIRONMENT=node -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS=['_f0r_bundle_emscripten_run','_f0r_bundle_emscripten_output_digest','_f0r_bundle_emscripten_application_frame_count','_f0r_bundle_emscripten_parameter_change_count','_f0r_bundle_emscripten_output_count','_f0r_bundle_emscripten_output_digest_at','_f0r_bundle_emscripten_output_id_at','_f0r_bundle_emscripten_failure_id','_f0r_bundle_emscripten_failure_stage']")
  if(FREI0R_BUNDLE_EMSCRIPTEN_GC)
    set_property(TARGET bundle-registry-emscripten APPEND_STRING PROPERTY LINK_FLAGS
      " -Wl,--gc-sections")
  endif()
  if(FREI0R_BUNDLE_WASM_PTHREADS)
    set_property(TARGET bundle-registry-emscripten APPEND_STRING PROPERTY LINK_FLAGS
      " -pthread -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=2")
  elseif(FREI0R_BUNDLE_WASM_SIMD)
    set_property(TARGET bundle-registry-emscripten APPEND_STRING PROPERTY LINK_FLAGS
      " -msimd128")
  endif()
  add_executable(bundle-emscripten-consumer bundle-emscripten-consumer.c)
  target_link_libraries(bundle-emscripten-consumer PRIVATE frei0r-bundle)
  set_property(TARGET bundle-emscripten-consumer APPEND_STRING PROPERTY LINK_FLAGS
    " --no-entry -sMODULARIZE=1 -sEXPORT_NAME=createBundleConsumer -sENVIRONMENT=node -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS=['_f0r_bundle_emscripten_consumer_run']")
  if(FREI0R_BUNDLE_EMSCRIPTEN_GC)
    set_property(TARGET bundle-emscripten-consumer APPEND_STRING PROPERTY LINK_FLAGS
      " -Wl,--gc-sections")
  endif()
  find_program(FREI0R_NODE_EXECUTABLE node)
  if(FREI0R_NODE_EXECUTABLE)
    add_test(NAME bundle-registry-emscripten
      COMMAND ${FREI0R_NODE_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/run-bundle-registry-emscripten.mjs
        $<TARGET_FILE:bundle-registry-emscripten>)
    add_test(NAME bundle-emscripten-consumer
      COMMAND ${FREI0R_NODE_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/run-bundle-emscripten-consumer.mjs
        $<TARGET_FILE:bundle-emscripten-consumer>)
    if(FREI0R_BUNDLE_EMSCRIPTEN_SCALAR_REFERENCE)
      add_test(NAME bundle-emscripten-profile-parity
        COMMAND ${FREI0R_NODE_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/compare-bundle-emscripten-profiles.mjs
          ${FREI0R_BUNDLE_EMSCRIPTEN_SCALAR_REFERENCE}
          $<TARGET_FILE:bundle-registry-emscripten>)
    endif()
  endif()
else()
  option(FREI0R_RUNTIME_SMOKE_TEST "Register the native archive smoke test with CTest" OFF)
  add_executable(runtime-smoke runtime-smoke.c
    $<TARGET_OBJECTS:runtime-smoke-registry>
  )
  target_link_libraries(runtime-smoke PRIVATE runtime-smoke-plugins)
  if(FREI0R_RUNTIME_SMOKE_TEST)
    add_test(NAME runtime-smoke COMMAND runtime-smoke)
  endif()
endif()

if(WIN32)
  find_package(dlfcn-win32 REQUIRED)
  target_link_libraries(frei0r-run PRIVATE dlfcn-win32::dl)
  target_link_libraries(frei0r-meta PRIVATE dlfcn-win32::dl)
else()
  target_link_libraries(frei0r-run PRIVATE m ${CMAKE_DL_LIBS})
  target_link_libraries(frei0r-meta PRIVATE m ${CMAKE_DL_LIBS})
endif()

if(TEST_ASAN)
  target_compile_options(frei0r-run PUBLIC
    -O0 -g -ggdb -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope)
  target_link_options(frei0r-run PUBLIC
    -fsanitize=address -fsanitize-recover=address
  )
  if(APPLE)
    target_link_options(frei0r-run PUBLIC
      -undefined dynamic_lookup
    )
  endif()
endif()

if(TEST_GUI)
  find_package(X11)
  if(X11_FOUND)
    target_compile_definitions(frei0r-run PRIVATE -DGUI)
    target_link_libraries(frei0r-run ${X11_LIBRARIES})
    target_include_directories(frei0r-run PRIVATE ${X11_INCLUDE_DIR})
  endif()
endif()

if(X11_FOUND)
  set(TESTFLAGS "-d -g")
else()
  set(TESTFLAGS "-d")
endif()

# find all targets and add tests
get_property(TDIRS DIRECTORY "${CMAKE_SOURCE_DIR}/src" PROPERTY SUBDIRECTORIES)
foreach(tdir ${TDIRS})
  get_property(DIRS DIRECTORY "${tdir}" PROPERTY SUBDIRECTORIES)
  foreach(dir ${DIRS})
    get_directory_property(targets DIRECTORY "${dir}" BUILDSYSTEM_TARGETS)
    foreach(target ${targets})
      # Targeted plugin builds must also produce the runner used to inspect them.
      add_dependencies(${target} frei0r-run)
      add_test(
        NAME "${target}"
        COMMAND "${CMAKE_BINARY_DIR}/test/frei0r-run" ${TESTFLAGS} -p "$<TARGET_FILE:${target}>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test"
      )
    endforeach()
  endforeach()
endforeach()

set(EXTENSION "so")
add_custom_target(generate-metadata
  COMMAND sh ${CMAKE_SOURCE_DIR}/test/extract-plugin-info.sh ${EXTENSION} "${CMAKE_BINARY_DIR}/src"
)
