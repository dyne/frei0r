set(SOURCES whirlpinch0r.c)
set(TARGET whirlpinch0r)

if(MSVC)
    list(APPEND SOURCES ${FREI0R_DEF})
endif()

add_library(${TARGET} MODULE ${SOURCES})
target_compile_options(${TARGET} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -ffast-math -fno-math-errno>
)
set_target_properties(${TARGET} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "whirlpinch0r"
)

install(TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
