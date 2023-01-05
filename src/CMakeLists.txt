
add_executable(chproc chproc.cpp)

add_library(scanner STATIC scanner.cpp vmmap.cpp)
target_include_directories(scanner PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(mypower mypower.cpp processlist.cpp)
target_link_libraries(mypower PRIVATE scanner tui dsl Boost::program_options)
