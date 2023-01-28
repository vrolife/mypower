
include(ExternalProject)

ExternalProject_Add(
    build_zstd
    URL https://github.com/facebook/zstd/releases/download/v1.5.2/zstd-1.5.2.tar.gz
    URL_MD5 072b10f71f5820c24761a65f31f43e73
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/.download
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/.source/zstd
    INSTALL_DIR ${TARGET_PROJECT_INSTALL_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND 
        AR=${CMAKE_AR}
        RANLIB=${CMAKE_RANLIB}
        STRIP=${CMAKE_STRIP}
        CC=${CMAKE_C_COMPILER}
        CXX=${CMAKE_CXX_COMPILER}
        CFLAGS=${ANDROID_TARGET_OPTS}
        make -C ${CMAKE_SOURCE_DIR}/.source/zstd/lib lib-release
    INSTALL_COMMAND 
        DESTDIR=${TARGET_PROJECT_INSTALL_DIR}
        PREFIX=/
        make -C ${CMAKE_SOURCE_DIR}/.source/zstd/lib install-static install-includes
)

add_library(zstd_zstd STATIC IMPORTED GLOBAL)
add_library(ZSTD::zstd ALIAS zstd_zstd)
add_dependencies(zstd_zstd build_zstd)
        
set_target_properties(zstd_zstd PROPERTIES 
    IMPORTED_LOCATION ${TARGET_PROJECT_INSTALL_DIR}/lib/libzstd.a
    INTERFACE_INCLUDE_DIRECTORIES ${TARGET_PROJECT_INSTALL_DIR}/include
)

# ninja-build bug need this target
add_custom_target(build_zstd_bug
    COMMAND ""
    BYPRODUCTS ${TARGET_PROJECT_INSTALL_DIR}/lib/libzstd.a
    DEPENDS build_zstd
)
