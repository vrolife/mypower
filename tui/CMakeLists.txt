
add_library(tui STATIC tui.cpp)
target_include_directories(tui PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(tui PUBLIC NCurses::ncurses)

add_executable(tui_example tui_example.cpp)
target_link_libraries(tui_example PRIVATE tui)
