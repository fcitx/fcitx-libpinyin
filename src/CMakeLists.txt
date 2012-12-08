find_package(PkgConfig REQUIRED)

include_directories (
    ${PROJECT_BINARY_DIR}
    ${LIBPINYIN_INCLUDE_DIR}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${LIBINTL_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    )

link_directories (${LIBPINYIN_LIBRARY_DIRS})

set( fcitx_libpinyin_sources
    eim.cpp
    enummap.cpp
    config.cpp
    pystring.cpp
    )

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon_full(libpinyin DESC
SOURCES ${fcitx_libpinyin_sources}
IM_CONFIG pinyin-libpinyin.conf shuangpin-libpinyin.conf zhuyin-libpinyin.conf
LINK_LIBS ${LIBPINYIN_LIBRARIES} ${LIBINTL_LIBRARIES} ${GLIB2_LIBRARIES}
)
