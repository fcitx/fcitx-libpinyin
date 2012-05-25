# - Try to find the Libpinyin libraries
# Once done this will define
#
#  LIBPINYIN_FOUND - system has LIBPINYIN
#  LIBPINYIN_INCLUDE_DIR - the LIBPINYIN include directory
#  LIBPINYIN_LIBRARIES - LIBPINYIN library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBPINYIN_INCLUDE_DIR AND LIBPINYIN_LIBRARIES)
    # Already in cache, be silent
    set(LIBPINYIN_FIND_QUIETLY TRUE)
endif(LIBPINYIN_INCLUDE_DIR AND LIBPINYIN_LIBRARIES)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBLIBPINYIN "libpinyin >= 0.5.0")
_pkgconfig_invoke("libpinyin" PC_LIBLIBPINYIN LIBPINYININCLUDEDIR "" "--variable=libpinyinincludedir")
_pkgconfig_invoke("libpinyin" LIBPINYIN PKGDATADIR "" "--variable=pkgdatadir")
_pkgconfig_invoke("libpinyin" LIBPINYIN EXECPREFIX "" "--variable=exec_prefix")

find_path(LIBPINYIN_MAIN_INCLUDE_DIR
          NAMES pinyin.h
          HINTS ${PC_LIBLIBPINYIN_LIBPINYININCLUDEDIR})

find_library(LIBPINYIN_LIBRARIES
             NAMES pinyin
             HINTS ${PC_LIBLIBPINYIN_LIBDIR})

set(LIBPINYIN_INCLUDE_DIR "${LIBPINYIN_MAIN_INCLUDE_DIR}")

find_program(LIBPINYIN_GEN_BINARY_FILES gen_binary_files HINTS "${LIBPINYIN_EXECPREFIX}/bin")
find_program(LIBPINYIN_GEN_UNIGRAM gen_unigram HINTS "${LIBPINYIN_EXECPREFIX}/bin")
find_program(LIBPINYIN_IMPORT_INTERPOLATION import_interpolation HINTS "${LIBPINYIN_EXECPREFIX}/bin")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libpinyin  DEFAULT_MSG 
                                  LIBPINYIN_LIBRARIES
                                  LIBPINYIN_MAIN_INCLUDE_DIR
                                  LIBPINYIN_PKGDATADIR
                                  LIBPINYIN_GEN_BINARY_FILES
                                  LIBPINYIN_GEN_UNIGRAM
                                  LIBPINYIN_IMPORT_INTERPOLATION
                                  )

mark_as_advanced(LIBPINYIN_INCLUDE_DIR LIBPINYIN_LIBRARIES)
