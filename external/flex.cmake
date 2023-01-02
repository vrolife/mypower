
include(ExternalProject)

ExternalProject_Add(
    flex_host
    URL https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz
    URL_MD5 2882e3179748cc9f9c23ec593d6adc8d
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/.download
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/.source/flex_host
    INSTALL_DIR ${HOST_PROJECT_INSTALL_DIR}
    CONFIGURE_COMMAND 
        ${CMAKE_SOURCE_DIR}/.source/flex_host/configure 
            ${HOST_PROJECT_OPTIONS}
            --disable-shared
            --enable-static
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
)

ExternalProject_Add(
    flex
    URL https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz
    URL_MD5 2882e3179748cc9f9c23ec593d6adc8d
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/.download
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/.source/flex
    INSTALL_DIR ${TARGET_PROJECT_INSTALL_DIR}
    CONFIGURE_COMMAND 
        ${CMAKE_SOURCE_DIR}/.source/flex/configure 
            ${TARGET_PROJECT_OPTIONS}
            --disable-shared
            --enable-static
    BUILD_COMMAND make -C src install-includeHEADERS
    INSTALL_COMMAND make -C src install-includeHEADERS
)
