
include(ExternalProject)

ExternalProject_Add(
    ncurses
    URL https://salsa.debian.org/debian/ncurses/-/archive/debian/6.3-2/ncurses-debian-6.3-2.tar.gz
    URL_MD5 9d929656b23e4d5bf72fdfc41e18d591
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/.download
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/.source/ncurses
    INSTALL_DIR ${TARGET_PROJECT_INSTALL_DIR}
    CONFIGURE_COMMAND 
        ${CMAKE_SOURCE_DIR}/.source/ncurses/configure 
            ${TARGET_PROJECT_OPTIONS}
            --without-manpages
            --without-tests
            --disable-stripping
            --without-debug
            # --enable-overwrite
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
)

# ninja-build bug need this target
add_custom_target(build_ncurses
    COMMAND ""
    BYPRODUCTS ${TARGET_PROJECT_INSTALL_DIR}/lib/libncurses.a
    DEPENDS ncurses
)

macro(add_ncurses_library NAME)
    add_library(ncurses_${NAME} STATIC IMPORTED GLOBAL)
    add_library(NCurses::${NAME} ALIAS ncurses_${NAME})
    add_dependencies(ncurses_${NAME} ncurses)
            
    set_target_properties(ncurses_${NAME} PROPERTIES 
        IMPORTED_LOCATION ${TARGET_PROJECT_INSTALL_DIR}/lib/lib${NAME}.a
        INTERFACE_INCLUDE_DIRECTORIES ${TARGET_PROJECT_INSTALL_DIR}/include
    )
endmacro()

add_ncurses_library(ncurses)
add_ncurses_library(ncurses++)
add_ncurses_library(menu)
add_ncurses_library(panel)
add_ncurses_library(form)

target_link_libraries(ncurses_ncurses++ INTERFACE
    ncurses_ncurses
    ncurses_menu
    ncurses_form
    ncurses_panel
)

target_link_libraries(ncurses_menu INTERFACE ncurses_ncurses)
target_link_libraries(ncurses_panel INTERFACE ncurses_ncurses)
target_link_libraries(ncurses_form INTERFACE ncurses_ncurses)
