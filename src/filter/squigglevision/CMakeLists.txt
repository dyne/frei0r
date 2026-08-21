include(CheckCXXSourceCompiles)

set(SOURCES squigglevision.cpp)
set(TARGET squigglevision)

if(MSVC)
  set(SOURCES ${SOURCES} ${FREI0R_DEF})
endif()

add_library(${TARGET} MODULE ${SOURCES})
set_target_properties(${TARGET} PROPERTIES PREFIX "")

# multithreading based on the kaleid0sc0pe filter: std::async over row bands,
# with a serial fallback where std::future is unavailable.
if(NOT WIN32)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(${TARGET} PRIVATE Threads::Threads)
endif()

check_cxx_source_compiles("
  #include <future>
  int main() { std::future<void> f; return 0; }" HAS_WORKING_FUTURE)
if(NOT HAS_WORKING_FUTURE)
  target_compile_definitions(${TARGET} PRIVATE NO_FUTURE)
endif()

install(TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})