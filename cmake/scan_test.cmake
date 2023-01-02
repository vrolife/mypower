
function(_add_mypower_test)
    cmake_parse_arguments(TEST "" "SOURCE" "SOURCES;LIBRARIES" ${ARGN})
    
    string(REPLACE .cpp "" TARGET_NAME "${TEST_SOURCE}")

    add_executable(${TARGET_NAME} ${TEST_SOURCE} ${TEST_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE ${TEST_LIBRARIES})
    target_compile_definitions(${TARGET_NAME} PRIVATE -DJINX_FORCE_ASSERT=1)
    
    if (EXISTS "${TARGET_NAME}.cmake")
        include("${TARGET_NAME}.cmake")
    endif()

    add_test(NAME ${TARGET_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)
endfunction(_add_mypower_test)

function(mypower_scan_test)
    cmake_parse_arguments(TEST "" "PATTERN" "SOURCES;LIBRARIES" ${ARGN})

    if (NOT TEST_PATTERN)
        set(TEST_PATTERN "test_*.cpp")
    endif()

    file(GLOB_RECURSE SOURCE_FILE_LIST LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ "${TEST_PATTERN}")

    foreach(SOURCE_FILE ${SOURCE_FILE_LIST})
        if (JINX_TEST_WHITE_LIST)
            if (SOURCE_FILE IN_LIST JINX_TEST_WHITE_LIST)
                _add_mypower_test(SOURCE ${SOURCE_FILE} SOURCES ${TEST_SOURCES} LIBRARIES ${TEST_LIBRARIES})
            endif()
        else()
            _add_mypower_test(SOURCE ${SOURCE_FILE} SOURCES ${TEST_SOURCES} LIBRARIES ${TEST_LIBRARIES})
        endif()
    endforeach()
endfunction()

enable_testing()
