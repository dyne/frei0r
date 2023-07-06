include(CheckIncludeFileCXX)
include(CheckCSourceCompiles)

cmake_policy(SET CMP0056 NEW)
cmake_policy(SET CMP0066 NEW)
cmake_policy(SET CMP0067 NEW)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set (SOURCES api.cpp ikaleid0sc0pe.h kaleid0sc0pe.cpp kaleid0sc0pe.h sse_mathfun_extension.h sse_mathfun.h)
set (TARGET kaleid0sc0pe)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

if (NOT WIN32)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET} PRIVATE Threads::Threads)
endif()

if(NO_SSE2)
    message(STATUS "SSE2 is disabled")
    add_definitions(-DNO_SSE2)
else()
    check_include_file_cxx(immintrin.h HAS_INTEL_INTRINSICS)
    if (HAS_INTEL_INTRINSICS)
        add_definitions(-DHAS_INTEL_INTRINSICS)
        if (NOT WIN32)
            target_compile_options(${TARGET} PRIVATE -msse2)
            set(CMAKE_REQUIRED_FLAGS "-msse2")
        endif()
        check_c_source_compiles("
                                    #include <emmintrin.h>
                                    #include <immintrin.h>
                                    int main(){ __m128 a = _mm_set1_ps(0.0f); a = _mm_sin_ps(a); return 0;}" HAS_SIN_INTRINSIC)
        check_c_source_compiles("
                                    #include <emmintrin.h>
                                    #include <immintrin.h>
                                    int main(){ __m128 a = _mm_set1_ps(0.0f); a = _mm_cos_ps(a); return 0;}" HAS_COS_INTRINSIC)
        check_c_source_compiles("
                                    #include <emmintrin.h>
                                    #include <immintrin.h>
                                    int main(){ __m128 a = _mm_set1_ps(0.0f); __m128 b = _mm_set1_ps(0.0f); a = _mm_atan2_ps(a,b); return 0;}" HAS_ATAN2_INTRINSIC)
        if (HAS_SIN_INTRINSIC)
            add_definitions(-DHAS_SIN_INTRINSIC)
        endif()
        if (HAS_COS_INTRINSIC)
            add_definitions(-DHAS_COS_INTRINSIC)
        endif()
        if (HAS_ATAN2_INTRINSIC)
            add_definitions(-DHAS_ATAN2_INTRINSIC)
        endif()
    endif()
endif()

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
