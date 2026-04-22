option(TEST_ASAN "Run tests in asan mode" OFF)
option(TEST_GUI "Run tests in gui mode" OFF)

add_executable(frei0r-run test-pattern.c frei0r-run.c)
add_executable(frei0r-meta frei0r-meta.c)

if(WIN32)
  find_package(dlfcn-win32 REQUIRED)
  target_link_libraries(frei0r-run PRIVATE dlfcn-win32::dl)
  target_link_libraries(frei0r-meta PRIVATE dlfcn-win32::dl)
else()
  target_link_libraries(frei0r-run PRIVATE m dl)
  target_link_libraries(frei0r-meta PRIVATE m dl)
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
