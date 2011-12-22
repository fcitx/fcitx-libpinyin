find_package(PkgConfig REQUIRED)

include_directories (
    ${PROJECT_BINARY_DIR}
    ${LIBPINYIN_INCLUDE_DIRS}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${LIBINTL_INCLUDE_DIR}
    )

link_directories (${LIBPINYIN_LIBRARY_DIRS})

set( fcitx_libpinyin_sources
    eim.cpp
   
    config.cpp
    )

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon(fcitx-libpinyin ${fcitx_libpinyin_sources} )
target_link_libraries (fcitx-libpinyin
                       ${LIBPINYIN_LIBRARIES}
                       ${LIBINTL_LIBRARIES}
                       )
