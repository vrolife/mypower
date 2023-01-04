
include(ExternalProject)

set(TERMLIST
    wsvt25m wsvt25
    pcansi
    screen-bce screen.xterm-256color screen-w screen-256color screen-s screen sun screen-256color-bce
    dumb
    mach-gnu-color mach-gnu mach-color mach-bold mach
    tmux-256color tmux
    linux
    ansi
    xterm xterm-256color xterm-r5 xterm-mono xterm-r6 xterm-xfree86 xterm-vt220 xterm-color
    hurd
    Eterm Eterm-color
    vt220 vt52 vt102 vt100
    rxvt rxvt-basic
    cons25-debian cons25
    cygwin
)
list(JOIN TERMLIST "," FALLBACKS)

if (ANDROID_PLATFORM)
    set(NCURSES_CONFIG_OPTS
        --with-terminfo-dirs=/system/etc/terminfo:/system_ext/etc/terminfo:/etc/terminfo
        --with-default-terminfo-dir=/etc/terminfo
        --with-fallbacks=xterm-256color,xterm,vt100)
else()
    set(NCURSES_CONFIG_OPTS
        --with-terminfo-dirs=/usr/share/terminfo:/usr/lib/terminfo:/etc/terminfo
        --with-default-terminfo-dir=/usr/share/terminfo
        --with-fallbacks=xterm-old,xterm-new,vt100)
endif()

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
            --disable-db-install
            ${NCURSES_CONFIG_OPTS}
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
