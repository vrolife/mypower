cmake_minimum_required(VERSION 3.16)

if(ANDROID_PLATFORM)
    set(CMAKE_TOOLCHAIN_FILE $ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake)
endif()

project(mypower)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/scan_test.cmake)

# add_subdirectory(simdjson)
add_subdirectory(external)
add_subdirectory(examples)
add_subdirectory(src)
add_subdirectory(dsl)
add_subdirectory(tui)

add_subdirectory(tests)
