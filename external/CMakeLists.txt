
set(TARGET_PROJECT_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install CACHE STRING "")
make_directory(${TARGET_PROJECT_INSTALL_DIR}/include)
make_directory(${TARGET_PROJECT_INSTALL_DIR}/lib)

set(HOST_PROJECT_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/host CACHE STRING "")
make_directory(${HOST_PROJECT_INSTALL_DIR}/include)
make_directory(${HOST_PROJECT_INSTALL_DIR}/lib)

set(TARGET_PROJECT_OPTIONS --prefix=${TARGET_PROJECT_INSTALL_DIR})
set(HOST_PROJECT_OPTIONS --prefix=${HOST_PROJECT_INSTALL_DIR})

if(ANDROID_PLATFORM)
    string(REPLACE - "" ANDROID_PLATFORM_ ${ANDROID_PLATFORM})
    set(ANDROID_TARGET aarch64-linux-${ANDROID_PLATFORM_})
    set(ANDROID_TARGET_OPTS --target=aarch64-linux-${ANDROID_PLATFORM_})
    set(TARGET_PROJECT_OPTIONS
        ${TARGET_PROJECT_OPTIONS}
        --host=${ANDROID_TARGET}
        AR=${CMAKE_AR}
        RANLIB=${CMAKE_RANLIB}
        STRIP=${CMAKE_STRIP}
        CC=${CMAKE_C_COMPILER}
        CXX=${CMAKE_CXX_COMPILER}
        "CFLAGS=--target=${ANDROID_TARGET} -I${TARGET_PROJECT_INSTALL_DIR}/include"
        "LDFLAGS=-L${TARGET_PROJECT_INSTALL_DIR}/lib"
    )
endif()

include(flex.cmake)
include(ncurses.cmake)
include(boost.cmake)
# include(zstd.cmake)

add_subdirectory(sljit)

add_boost_libs(program_options json VERSION 1.81.0)
