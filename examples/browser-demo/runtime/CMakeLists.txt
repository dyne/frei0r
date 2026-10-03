add_library(frei0r-demo-contract STATIC frei0r-demo-contract.c)
target_include_directories(frei0r-demo-contract PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(frei0r-demo-adapter OBJECT frei0r-demo-adapter.c)
target_include_directories(frei0r-demo-adapter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(frei0r-demo-adapter PRIVATE frei0r-demo-contract frei0r-bundle)

if(BUILD_TESTING)
  add_executable(demo-wasm-contract-test demo-wasm-contract-test.c)
  target_link_libraries(demo-wasm-contract-test PRIVATE
    frei0r-demo-contract frei0r-bundle)
  add_test(NAME demo-wasm-contract COMMAND demo-wasm-contract-test)

  add_executable(demo-wasm-adapter-test demo-wasm-adapter-test.c
    $<TARGET_OBJECTS:frei0r-demo-adapter>)
  target_link_libraries(demo-wasm-adapter-test PRIVATE
    frei0r-demo-contract frei0r-bundle)
  add_test(NAME demo-wasm-adapter COMMAND demo-wasm-adapter-test)
endif()

if(EMSCRIPTEN)
  add_executable(frei0r-demo-browser-runtime frei0r-demo-browser-runtime.c
    $<TARGET_OBJECTS:frei0r-demo-adapter>)
  target_link_libraries(frei0r-demo-browser-runtime PRIVATE
    frei0r-demo-contract frei0r-bundle)
  set_target_properties(frei0r-demo-browser-runtime PROPERTIES
    OUTPUT_NAME frei0r-demo-runtime SUFFIX ".mjs")
  set_property(TARGET frei0r-demo-browser-runtime APPEND_STRING PROPERTY LINK_FLAGS
    " --no-entry -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createFrei0rDemoRuntime -sENVIRONMENT=web,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sINITIAL_MEMORY=33554432 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_FUNCTIONS=['_frei0r_demo_catalog_count','_frei0r_demo_catalog_id','_frei0r_demo_catalog_name','_frei0r_demo_catalog_author','_frei0r_demo_catalog_explanation','_frei0r_demo_catalog_color_model','_frei0r_demo_catalog_parameter_count','_frei0r_demo_select','_frei0r_demo_input_pointer','_frei0r_demo_output_pointer','_frei0r_demo_parameter_count','_frei0r_demo_parameter_type','_frei0r_demo_parameter_name','_frei0r_demo_parameter_explanation','_frei0r_demo_get_parameter_scalar','_frei0r_demo_set_parameter_scalar','_frei0r_demo_get_parameter_color_component','_frei0r_demo_set_parameter_color','_frei0r_demo_get_parameter_position_component','_frei0r_demo_set_parameter_position','_frei0r_demo_reset_parameters','_frei0r_demo_update','_frei0r_demo_last_error','_frei0r_demo_shutdown']")
  find_program(FREI0R_NODE_EXECUTABLE node)
  if(FREI0R_NODE_EXECUTABLE)
    add_test(NAME demo-wasm-adapter-emscripten
      COMMAND ${FREI0R_NODE_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/run-demo-wasm-runtime.mjs
        $<TARGET_FILE:frei0r-demo-browser-runtime>)
    add_test(NAME demo-wasm-measure-emscripten
      COMMAND ${FREI0R_NODE_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/measure-demo-wasm-runtime.mjs
        $<TARGET_FILE:frei0r-demo-browser-runtime> --verify)
  endif()
endif()
