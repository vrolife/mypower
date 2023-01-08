
# MathExpression
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.flex
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_parser.hpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py 
        --classname MathExpression
        --namespace mathexpr
        --include mathexpr.hpp
        --tokenizer ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.hpp 
        --parser ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_parser.hpp
        --flex ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.flex
    COMMAND ${HOST_PROJECT_INSTALL_DIR}/bin/flex 
        -o ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.cpp 
        ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.flex
)

# ComparatorExpression
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.flex
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compexpr_parser.hpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py 
        --classname ComparatorExpression
        --namespace compexpr
        --include compexpr.hpp
        --tokenizer ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.hpp 
        --parser ${CMAKE_CURRENT_BINARY_DIR}/compexpr_parser.hpp
        --flex ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.flex
    COMMAND ${HOST_PROJECT_INSTALL_DIR}/bin/flex 
        -o ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.cpp 
        ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.flex
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

add_library(dsl STATIC mathexpr.cpp cmd.cpp dsl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/compexpr_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.hpp
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

add_executable(chproc chproc.cpp)

add_library(scanner STATIC process.cpp vmmap.cpp)
target_include_directories(scanner PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(commands STATIC scan.cpp)
target_link_libraries(commands PRIVATE scanner tui dsl Boost::program_options)

add_executable(mypower mypower.cpp processlist.cpp)
target_link_libraries(mypower PRIVATE commands scanner tui dsl Boost::program_options)
