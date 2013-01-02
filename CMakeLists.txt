cmake_minimum_required(VERSION 2.6)
project(fcitx-libpinyin)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

FIND_PACKAGE(Libintl REQUIRED)
FIND_PACKAGE(Fcitx 4.2.7 REQUIRED)
find_package(GLIB2 REQUIRED)
find_package(Libpinyin REQUIRED)
find_program(TAR tar REQUIRED)

_fcitx_add_uninstall_target()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" FCITX4_MAJOR_VERSION "${FCITX4_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" FCITX4_MINOR_VERSION "${FCITX4_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" FCITX4_PATCH_VERSION "${FCITX4_VERSION}")

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

set(FCITX_LIBPINYIN_ZHUYIN_DATADIR
  "${FCITX4_PREFIX}/share/fcitx/libpinyin/zhuyin_data")
set(libdir ${LIB_INSTALL_DIR})

configure_file(config.h.in config.h)

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(data)

add_subdirectory(standalone)
