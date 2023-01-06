
# AddressExpr
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.flex
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/expr_parser.hpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py 
        --classname AddressExpr
        --namespace expr
        --include expr.hpp
        --tokenizer ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.hpp 
        --parser ${CMAKE_CURRENT_BINARY_DIR}/expr_parser.hpp
        --flex ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.flex
    COMMAND ${HOST_PROJECT_INSTALL_DIR}/bin/flex 
        -o ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.cpp 
        ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.flex
)

# Comparator
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.flex
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comparator_parser.hpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py 
        --classname Comparator
        --namespace comparator
        --include comparator.hpp
        --tokenizer ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.hpp 
        --parser ${CMAKE_CURRENT_BINARY_DIR}/comparator_parser.hpp
        --flex ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.flex
    COMMAND ${HOST_PROJECT_INSTALL_DIR}/bin/flex 
        -o ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.cpp 
        ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.flex
)

# CommandParser
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.flex
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmd_parser.hpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py 
        --classname CommandParser
        --namespace cmd
        --include cmd.hpp
        --tokenizer ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.hpp 
        --parser ${CMAKE_CURRENT_BINARY_DIR}/cmd_parser.hpp
        --flex ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.flex
    COMMAND ${HOST_PROJECT_INSTALL_DIR}/bin/flex 
        -o ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.cpp 
        ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.flex
)

add_library(dsl STATIC expr.cpp cmd.cpp dsl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/expr_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/expr_tokenizer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/comparator_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/comparator_tokenizer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/cmd_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/cmd_tokenizer.hpp
)

target_link_libraries(dsl PUBLIC sljit)

execute_process(
    COMMAND python3 -m playlang.cplusplus
    OUTPUT_VARIABLE PLAYLANG_CPP_HEADERS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_include_directories(dsl PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLAYLANG_CPP_HEADERS}
    ${TARGET_PROJECT_INSTALL_DIR}/include
)

add_dependencies(dsl flex flex_host)

mypower_scan_test(PATTERN "test_*" LIBRARIES dsl)

add_executable(chproc chproc.cpp)

add_library(scanner STATIC process.cpp vmmap.cpp)
target_include_directories(scanner PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(mypower mypower.cpp processlist.cpp scan.cpp)
target_link_libraries(mypower PRIVATE scanner tui dsl Boost::program_options)
