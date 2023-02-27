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

if (NOT EAZY_BOOST_DISABLE_GLOBAL_CACHE)
set(EAZY_BOOST_DATA_DIR $ENV{HOME}/.boost)
make_directory(${EAZY_BOOST_DATA_DIR})
else()
set(EAZY_BOOST_DATA_DIR ${CMAKE_SOURCE_DIR})
endif()

set(EAZY_BOOST_DOWNLOAD_CACHE_DIR ${EAZY_BOOST_DATA_DIR}/.download)
set(EAZY_BOOST_SOURCE_CACHE_DIR ${EAZY_BOOST_DATA_DIR}/.source)
set(EAZY_BOOST_GETDEPS_LIST_FILE ${CMAKE_CURRENT_LIST_FILE})

file(MAKE_DIRECTORY ${EAZY_BOOST_DOWNLOAD_CACHE_DIR})
file(MAKE_DIRECTORY ${EAZY_BOOST_SOURCE_CACHE_DIR})

function(_add_boost_lib LIB VERSION)
    set(DOWNLOAD_URL https://github.com/boostorg/${LIB}/archive/refs/tags/boost-${VERSION}.tar.gz)
    set(LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost/${LIB})
    set(ARCHIVE ${EAZY_BOOST_DOWNLOAD_CACHE_DIR}/${LIB}-${VERSION}.tar.gz)
    set(SOURCE_DIR ${EAZY_BOOST_SOURCE_CACHE_DIR}/${LIB}-boost-${VERSION})

    if (EXISTS ${LIB_DIR}/getdeps/done.txt)
        return()
    endif()

    if (__boost_lib_parsing_${LIB})
        return()
    endif()

    set(__boost_lib_parsing_${LIB} ON)

    message(STATUS "Add ${LIB} ${ARCHIVE}")

    if (NOT EXISTS ${ARCHIVE})
        file(DOWNLOAD ${DOWNLOAD_URL} ${ARCHIVE})
    endif()

    if (NOT EXISTS ${SOURCE_DIR})
        execute_process(
            COMMAND cmake -E tar xf ${ARCHIVE}
            WORKING_DIRECTORY ${EAZY_BOOST_SOURCE_CACHE_DIR}
            RESULT_VARIABLE PROCESS_STATUS
            OUTPUT_QUIET
        )

        if(NOT EXISTS ${SOURCE_DIR})
            file(REMOVE ${ARCHIVE})
            message(FATAL_ERROR "Failed to extract ${ARCHIVE} please try again")
        endif()

        if(LIB STREQUAL property_tree)
            file(READ ${SOURCE_DIR}/CMakeLists.txt __content)
            string(REPLACE "if(BOOST_SUPERPROJECT_VERSION)" "if(ON)" __output "${__content}")
            file(WRITE ${SOURCE_DIR}/CMakeLists.txt "${__output}")
        endif()
    endif()
    
    if (NOT EXISTS ${LIB_DIR}/getdeps)
        # getdeps
        execute_process(
            COMMAND cmake -E copy ${EAZY_BOOST_GETDEPS_LIST_FILE} ${LIB_DIR}/CMakeLists.txt
            OUTPUT_QUIET
        )

        execute_process(
            COMMAND cmake -B ${LIB_DIR}/getdeps -S ${LIB_DIR} -DGETDEPS_SOURCE_DIR=${SOURCE_DIR}
            RESULT_VARIABLE PROCESS_STATUS
            OUTPUT_QUIET
        )

        if(NOT PROCESS_STATUS EQUAL 0)
            file(REMOVE_RECURSE ${LIB_DIR}/getdeps)
            message(FATAL_ERROR "Failed to get the depends of ${LIB}")
        endif()
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
    file(WRITE ${LIB_DIR}/getdeps/done.txt "DONE")
endfunction()

function(add_boost_libs)
    cmake_parse_arguments(BOOST "" "VERSION" "LIBRARIES" ${ARGN})

    if (NOT BOOST_VERSION)
        set(BOOST_VERSION 1.81.0)
    endif()

    foreach(LIB ${BOOST_UNPARSED_ARGUMENTS} ${BOOST_LIBRARIES})
        _add_boost_lib(${LIB} ${BOOST_VERSION})
    endforeach()
    
    file(GLOB PROJECTS LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_BINARY_DIR}/boost ${CMAKE_CURRENT_BINARY_DIR}/boost/*)
    foreach(PRJ ${PROJECTS})
        message(STATUS "${PRJ}")
        add_subdirectory(${EAZY_BOOST_SOURCE_CACHE_DIR}/${PRJ}-boost-${BOOST_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/boost/${PRJ})
    endforeach()
endfunction()
