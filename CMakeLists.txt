cmake_minimum_required(VERSION 3.6)
project(fcitx-libpinyin)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(ENABLE_QT "Enable Qt for dictmanager" On)

find_package(Libintl REQUIRED)
find_package(Fcitx 4.2.8 REQUIRED)
pkg_check_modules(DBus "dbus-1" REQUIRED IMPORTED_TARGET)
pkg_check_modules(LibPinyin "libpinyin>=2.6.0" REQUIRED IMPORTED_TARGET)
pkg_get_variable(LIBPINYIN_PKGDATADIR "libpinyin" "pkgdatadir")
pkg_get_variable(LIBPINYIN_EXECPREFIX "libpinyin" "exec_prefix")
pkg_get_variable(LIBPINYIN_DATABASE_FORMAT "libpinyin" "database_format")

find_program(LIBPINYIN_GEN_BINARY_FILES gen_binary_files HINTS "${LIBPINYIN_EXECPREFIX}/bin")
find_program(LIBPINYIN_GEN_UNIGRAM gen_unigram HINTS "${LIBPINYIN_EXECPREFIX}/bin")
find_program(LIBPINYIN_IMPORT_INTERPOLATION import_interpolation HINTS "${LIBPINYIN_EXECPREFIX}/bin")

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

if (LIBPINYIN_GEN_UNIGRAM AND LIBPINYIN_GEN_BINARY_FILES AND LIBPINYIN_IMPORT_INTERPOLATION)
    set(LIBPINYIN_TOOLS_FOUND 1)
else()
    set(LIBPINYIN_TOOLS_FOUND 0)
endif ()

set(FCITX_LIBPINYIN_ZHUYIN_DATADIR
  "${FCITX4_PREFIX}/share/fcitx/libpinyin/zhuyin_data")
set(libdir ${LIB_INSTALL_DIR})

configure_file(config.h.in config.h)

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(dictmanager)
