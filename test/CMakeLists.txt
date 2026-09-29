option(TEST_ASAN "Run tests in asan mode" OFF)
option(TEST_GUI "Run tests in gui mode" OFF)

add_executable(frei0r-run test-pattern.c frei0r-run.c)
add_executable(frei0r-meta frei0r-meta.c)

# This intentionally compiles two plugins with private entry point names so
# that the archive/registry path can be exercised without changing the ABI of
# the ordinary MODULE targets.
add_library(runtime-smoke-brightness OBJECT
  ${CMAKE_SOURCE_DIR}/src/filter/brightness/brightness.c
)
target_compile_definitions(runtime-smoke-brightness PRIVATE
  f0r_init=brightness_f0r_init
  f0r_deinit=brightness_f0r_deinit
  f0r_get_plugin_info=brightness_f0r_get_plugin_info
  f0r_get_param_info=brightness_f0r_get_param_info
  f0r_construct=brightness_f0r_construct
  f0r_destruct=brightness_f0r_destruct
  f0r_set_param_value=brightness_f0r_set_param_value
  f0r_get_param_value=brightness_f0r_get_param_value
  f0r_update=brightness_f0r_update
)

add_library(runtime-smoke-invert0r OBJECT
  ${CMAKE_SOURCE_DIR}/src/filter/invert0r/invert0r.c
)
target_compile_definitions(runtime-smoke-invert0r PRIVATE
  f0r_init=invert0r_f0r_init
  f0r_deinit=invert0r_f0r_deinit
  f0r_get_plugin_info=invert0r_f0r_get_plugin_info
  f0r_get_param_info=invert0r_f0r_get_param_info
  f0r_construct=invert0r_f0r_construct
  f0r_destruct=invert0r_f0r_destruct
  f0r_set_param_value=invert0r_f0r_set_param_value
  f0r_get_param_value=invert0r_f0r_get_param_value
  f0r_update=invert0r_f0r_update
)

add_library(runtime-smoke-plugins STATIC
  $<TARGET_OBJECTS:runtime-smoke-brightness>
  $<TARGET_OBJECTS:runtime-smoke-invert0r>
)

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  option(FREI0R_RUNTIME_SMOKE_GC "Enable section garbage collection for the WASI smoke module" OFF)
  add_executable(runtime-smoke-wasi runtime-smoke.c runtime-smoke-registry.c)
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
  add_executable(runtime-smoke-emscripten runtime-smoke.c runtime-smoke-registry.c)
  target_compile_definitions(runtime-smoke-emscripten PRIVATE RUNTIME_SMOKE_NO_MAIN)
  target_link_libraries(runtime-smoke-emscripten PRIVATE runtime-smoke-plugins)
  set_property(TARGET runtime-smoke-emscripten APPEND_STRING PROPERTY LINK_FLAGS
    " --no-entry -sMODULARIZE=1 -sEXPORT_NAME=createRuntimeSmoke -sENVIRONMENT=node -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS=['_runtime_smoke_run']")
else()
  option(FREI0R_RUNTIME_SMOKE_TEST "Register the native archive smoke test with CTest" OFF)
  add_executable(runtime-smoke runtime-smoke.c runtime-smoke-registry.c)
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
