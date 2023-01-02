if(GETDEPS_SOURCE_DIR)
cmake_minimum_required(VERSION 3.4)
project(getdeps)

file(REMOVE ${CMAKE_BINARY_DIR}/deps.txt)

macro(target_link_libraries TARGET)
    cmake_parse_arguments(MY "" "" "PRIVATE;PUBLIC;INTERFACE" ${ARGN})

    if (MY_INTERFACE)
        list(APPEND GETDEPS_LIST ${MY_INTERFACE})
    endif()
    
    if (MY_PRIVATE)
        list(APPEND GETDEPS_LIST ${MY_PRIVATE})
    endif()

    if (MY_PUBLIC)
        list(APPEND GETDEPS_LIST ${MY_PUBLIC})
    endif()
    
    file(WRITE ${CMAKE_BINARY_DIR}/deps.txt "${GETDEPS_LIST}")

endmacro()

add_subdirectory(${GETDEPS_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/project)
endif()

set(EAZY_BOOST_DOWNLOAD_CACHE_DIR ${CMAKE_SOURCE_DIR}/.download)
set(EAZY_BOOST_SOURCE_CACHE_DIR ${CMAKE_SOURCE_DIR}/.source)
set(EAZY_BOOST_GETDEPS_LIST_FILE ${CMAKE_CURRENT_LIST_FILE})

file(MAKE_DIRECTORY ${EAZY_BOOST_DOWNLOAD_CACHE_DIR})
file(MAKE_DIRECTORY ${EAZY_BOOST_SOURCE_CACHE_DIR})

function(_add_boost_lib LIB VERSION)
    set(DOWNLOAD_URL https://github.com/boostorg/${LIB}/archive/refs/tags/boost-${VERSION}.tar.gz)
    set(LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost/${LIB})
    set(ARCHIVE ${EAZY_BOOST_DOWNLOAD_CACHE_DIR}/${LIB}.tar.gz)

    if (NOT EXISTS ${ARCHIVE})
        file(DOWNLOAD ${DOWNLOAD_URL} ${ARCHIVE})
    endif()

    set(SOURCE_DIR ${EAZY_BOOST_SOURCE_CACHE_DIR}/${LIB}-boost-${VERSION})

    if (NOT EXISTS ${SOURCE_DIR})
        execute_process(
            COMMAND cmake -E tar xf ${ARCHIVE}
            WORKING_DIRECTORY ${EAZY_BOOST_SOURCE_CACHE_DIR}
            RESULT_VARIABLE PROC_CODE
            OUTPUT_QUIET
        )
    endif()
    
    if (NOT EXISTS ${LIB_DIR}/getdeps)
        # getdeps
        execute_process(
            COMMAND cmake -E copy ${EAZY_BOOST_GETDEPS_LIST_FILE} ${LIB_DIR}/CMakeLists.txt
            OUTPUT_QUIET
        )

        execute_process(
            COMMAND cmake -B ${LIB_DIR}/getdeps -S ${LIB_DIR} -DGETDEPS_SOURCE_DIR=${SOURCE_DIR}
            OUTPUT_QUIET
        )
    endif()

    if (EXISTS ${LIB_DIR}/getdeps/deps.txt)
        file(READ ${LIB_DIR}/getdeps/deps.txt DEPS)
        foreach(DEP ${DEPS})
            if (DEP MATCHES "Boost::.*")
                string(REPLACE "Boost::" "" DEP_LIB ${DEP})
                _add_boost_lib(${DEP_LIB} ${VERSION})
            endif()
        endforeach()
    endif()
    
endfunction()

function(add_boost_libs)
    cmake_parse_arguments(BOOST "" "VERSION" "LIBRARIES" ${ARGN})

    if (NOT BOOST_VERSION)
        set(BOOST_VERSION 1.81.0)
    endif()

    foreach(LIB ${BOOST_UNPARSED_ARGUMENTS} ${BOOST_LIBRARIES})
        message(STATUS "Add ${LIB}")
        _add_boost_lib(${LIB} ${BOOST_VERSION})
    endforeach()
    

    file(GLOB PROJECTS LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_BINARY_DIR}/boost ${CMAKE_CURRENT_BINARY_DIR}/boost/*)
    foreach(PRJ ${PROJECTS})
        message(STATUS "${PRJ}")
        add_subdirectory(${EAZY_BOOST_SOURCE_CACHE_DIR}/${PRJ}-boost-${BOOST_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/${PRJ})
    endforeach()
endfunction()
