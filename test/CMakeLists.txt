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

if(TARGET frei0r-bundle)
  add_executable(bundle-native-consumer bundle-native-consumer.c)
  target_link_libraries(bundle-native-consumer PRIVATE frei0r-bundle)
  add_test(NAME bundle-native-consumer COMMAND bundle-native-consumer)

  add_test(NAME frei0r-bundle-shared-exports
    COMMAND ${CMAKE_COMMAND}
      -DPROBE=$<TARGET_FILE:frei0r-bundle-shared>
      -DNM=${CMAKE_NM}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/check-bundle-symbols.cmake
  )
endif()

add_executable(bundle-api-contract bundle-api-contract.c
  $<TARGET_OBJECTS:runtime-smoke-registry>
)
target_link_libraries(bundle-api-contract PRIVATE runtime-smoke-plugins)
add_test(NAME bundle-api-contract COMMAND bundle-api-contract)

add_executable(bundle-api-cxx bundle-api-cxx.cpp)
add_test(NAME bundle-api-cxx COMMAND bundle-api-cxx)

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  option(FREI0R_RUNTIME_SMOKE_GC "Enable section garbage collection for the WASI smoke module" OFF)
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
elseif(EMSCRIPTEN)
  add_executable(runtime-smoke-emscripten runtime-smoke.c
    $<TARGET_OBJECTS:runtime-smoke-registry>
  )
  target_compile_definitions(runtime-smoke-emscripten PRIVATE RUNTIME_SMOKE_NO_MAIN)
  target_link_libraries(runtime-smoke-emscripten PRIVATE runtime-smoke-plugins)
  set_property(TARGET runtime-smoke-emscripten APPEND_STRING PROPERTY LINK_FLAGS
    " --no-entry -sMODULARIZE=1 -sEXPORT_NAME=createRuntimeSmoke -sENVIRONMENT=node -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS=['_runtime_smoke_run']")
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
