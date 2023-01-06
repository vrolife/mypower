project(sljit C)

add_library(sljit STATIC
    sljit_src/sljitLir.c
)
target_include_directories(sljit PUBLIC sljit_src)
