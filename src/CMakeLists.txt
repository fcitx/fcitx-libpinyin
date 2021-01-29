
include_directories (
    ${PROJECT_BINARY_DIR}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${LIBINTL_INCLUDE_DIR}
    )

set( fcitx_libpinyin_sources
    eim.cpp
    enummap.cpp
    config.cpp
    bus.cpp
    utils.cpp
    )

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon_full(libpinyin DESC
SOURCES ${fcitx_libpinyin_sources}
IM_CONFIG pinyin-libpinyin.conf shuangpin-libpinyin.conf zhuyin-libpinyin.conf
LINK_LIBS PkgConfig::LibPinyin PkgConfig::DBus ${LIBINTL_LIBRARIES}
)
