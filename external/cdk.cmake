
include(ExternalProject)

ExternalProject_Add(
    cdk
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/cdk
    INSTALL_DIR ${TARGET_PROJECT_INSTALL_DIR}
    CONFIGURE_COMMAND 
        ${CMAKE_CURRENT_LIST_DIR}/cdk/configure 
            ${TARGET_PROJECT_OPTIONS}
            --without-shared
            --with-curses-dir=${TARGET_PROJECT_INSTALL_DIR}/include/ncurses
            --with-ncurses
    BUILD_COMMAND make -j all examples
    INSTALL_COMMAND make install
)

function(add_cdk_library NAME)
    add_library(cdk_${NAME} STATIC IMPORTED GLOBAL)
    add_library(CDK::${NAME} ALIAS cdk_${NAME})
    add_dependencies(cdk_${NAME} cdk)
    set_target_properties(cdk_${NAME} PROPERTIES 
        IMPORTED_LOCATION ${TARGET_PROJECT_INSTALL_DIR}/lib/lib${NAME}.a
        INTERFACE_INCLUDE_DIRECTORIES ${TARGET_PROJECT_INSTALL_DIR}/include
    )
endfunction()

add_cdk_library(cdk)
target_link_libraries(cdk_cdk INTERFACE ncurses_ncurses)
add_dependencies(cdk_cdk ncurses_ncurses)
