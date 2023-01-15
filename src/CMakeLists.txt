
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
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.flex
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmdline_parser.hpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dsl.py 
        --classname CommandParser
        --namespace cmdline
        --include cmdline.hpp
        --tokenizer ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.hpp 
        --parser ${CMAKE_CURRENT_BINARY_DIR}/cmdline_parser.hpp
        --flex ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.flex
    COMMAND ${HOST_PROJECT_INSTALL_DIR}/bin/flex 
        -o ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.cpp 
        ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.flex
)

add_library(dsl STATIC mathexpr.cpp cmdline.cpp dsl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/mathexpr_tokenizer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/compexpr_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/compexpr_tokenizer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/cmdline_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/cmdline_tokenizer.hpp
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

file(GLOB COMMAND_SOURCES cmd_*.cpp)

add_executable(mypower mypower.cpp ${COMMAND_SOURCES})
target_link_libraries(mypower PRIVATE scanner tui dsl Boost::program_options)

# add_executable(findref findref.cpp)
# target_link_libraries(findref PRIVATE scanner Boost::program_options)
